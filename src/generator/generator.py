"""This generator will validate json defined JSON as model via a schema and generate according C++."""
import json
import os
import sys
import jsonschema
import argparse
import yaml
from types import SimpleNamespace
from yaml.loader import SafeLoader
from jinja2 import Environment, FileSystemLoader
from constants import PREFIX_MAP
from validators import enum_validator, struct_validator, group_validator, data_point_validator, type_validator, processor_validator
from validatorException import DatapointException
from umlGenerator import generate_uml
from overviewGenerator import generate_overview
from modelMetadata import create_model_metadata

GENERATED_FOLDER = '/generated/include'
DOC_FOLDER = '/generated/doc'
PYTHON_FOLDER = '/generated/datalayer_example'

RTT_SCALAR_FORMATS = {
    'uint8_t': 'B',
    'uint16_t': 'H',
    'uint32_t': 'I',
    'uint64_t': 'Q',
    'int8_t': 'b',
    'int16_t': 'h',
    'int32_t': 'i',
    'int64_t': 'q',
    'float': 'f',
    'double': 'd',
    'bool': '?',
}

PERFECT_HASH_MASK = 0xffffffff


def perfect_hash(identifier, seed):
    """Return a deterministic 32-bit hash for a datapoint identifier and seed."""
    value = (identifier ^ seed) & PERFECT_HASH_MASK
    value ^= value >> 16
    value = (value * 0x7feb352d) & PERFECT_HASH_MASK
    value ^= value >> 15
    value = (value * 0x846ca68b) & PERFECT_HASH_MASK
    return (value ^ (value >> 16)) & PERFECT_HASH_MASK


def validate_json(model_data, schema):
    """
    Validate the given model json with defined schema.

    :param model_data: json model file content
    :param schema: json schema file content
    :return:
    """
    jsonschema.validate(instance=model_data, schema=schema)


def create_group_data_point_dict(dps):
    """
    Create a dictionary which will have group and data points belong to this group.

    :param dps: list of all data points
    :return: group data point mapping.
    """
    temp = dict()
    for dp in dps:
        group = dp['group']
        name = dp['name']
        if group not in temp:
            temp[group] = []
        if 'namespace' in dp and len(dp['namespace']) > 0:
            name = '{}::{}'.format(dp['namespace'], name)
        temp[group].append(name)
    return temp


def create_perfect_hash(data_points, groups):
    """Build a two-level FKS perfect hash for current and legacy absolute datapoint IDs."""
    group_ids = {group['name']: int(group['baseId'], 0) for group in groups}
    entries = []
    identifiers = set()

    for index, data_point in enumerate(data_points):
        for relative_id in [data_point['id'], *data_point['renamedFrom']]:
            identifier = group_ids[data_point['group']] + relative_id
            if identifier in identifiers:
                raise DatapointException(f"duplicate absolute datapoint ID '{identifier:#x}'")
            identifiers.add(identifier)
            entries.append({'id': identifier, 'index': index})

    bucket_count = len(entries)
    if bucket_count == 0:
        return {'bucket_count': 0, 'buckets': [], 'slots': []}

    bucket_entries = [[] for _ in range(bucket_count)]
    for entry in entries:
        bucket_entries[perfect_hash(entry['id'], 0) % bucket_count].append(entry)

    buckets = []
    slots = []
    for entries_in_bucket in bucket_entries:
        slot_count = len(entries_in_bucket) ** 2
        offset = len(slots)
        if slot_count == 0:
            buckets.append({'offset': offset, 'slot_count': 0, 'seed': 0})
            continue

        seed = 0
        while True:
            positions = [perfect_hash(entry['id'], seed) % slot_count for entry in entries_in_bucket]
            if len(positions) == len(set(positions)):
                break
            seed += 1

        bucket_slots = [{'id': 0, 'index': 0, 'occupied': False} for _ in range(slot_count)]
        for entry, position in zip(entries_in_bucket, positions):
            bucket_slots[position] = {'id': entry['id'], 'index': entry['index'], 'occupied': True}
        buckets.append({'offset': offset, 'slot_count': slot_count, 'seed': seed})
        slots.extend(bucket_slots)

    return {'bucket_count': bucket_count, 'buckets': buckets, 'slots': slots}


def create_rtt_datapoints(data_points, groups, enums, types):
    """Return wire metadata for the generated RTT reader."""
    group_ids = {group['name']: int(group['baseId'], 0) for group in groups}
    enum_types = {enum['name']: enum['type'] for enum in enums}
    aliases = {data_type['name']: data_type['type'] for data_type in types}
    rtt_datapoints = []
    for data_point in data_points:
        wire_type = aliases.get(data_point['type'], enum_types.get(data_point['type'], data_point['type']))
        name = f"{data_point['namespace']}::{data_point['name']}" if data_point['namespace'] else data_point['name']
        rtt_datapoints.append({
            'id': group_ids[data_point['group']] + data_point['id'],
            'name': name,
            'type': data_point['type'],
            'format': RTT_SCALAR_FORMATS.get(wire_type),
            'array_size': data_point['arraySize'],
            'access': data_point['access'],
        })
    return rtt_datapoints


def get_args():
    """
    Parse defined args and check for existence.

    :return: parsed args
    """
    parser = argparse.ArgumentParser(description='Generates code from defined model.')
    parser.add_argument('-s', '--model_dir', required=True, help='Model directory for input files.')
    parser.add_argument('-o', '--out_dir', required=True,
                        help='Out directory which point to the folder where the generated files will be saved.')
    parser.add_argument('-t', '--template_dir', required=True,
                        help='Template directory where the jinja2 templates are located.')
    parser.add_argument('-c', '--schema_dir', required=True, help='Scheme directory where the schema.json is located.')
    parser.add_argument('-n', '--module_name', required=False, default='datalayer_example',
                        help='Python module name used in the generated pybind11 binding (default: datalayer_example).')
    parser.add_argument('--check', action='store_true',
                        help='Validate the model and print a machine-readable summary without generating files.')
    if sys.version_info < (3, 9):
        parser.add_argument('-x', '--convert', action='store_true', help='flag for converting json to yaml')
    else:
        parser.add_argument('-x', '--convert', action=argparse.BooleanOptionalAction,
                            help='flag for converting json to yaml')
    return parser.parse_args()


def read_model_files(args):
    """
    Read all defined JSON files in model dir.

    :return: json_data of all JSON files
    """
    json_data = {'Enums': [], 'Groups': [], 'Structs': [], 'Datapoints': [], 'Processors': [], 'Types': []}
    for root, dirs, files in os.walk(f'{args.model_dir}'):
        for name in files:
            tmp_dict = None
            if name.endswith('.json'):
                with open(os.path.join(root, name)) as file:
                    tmp_dict = json.load(file)
                if args.convert:
                    name_without_json = name[:name.rfind('.json')]
                    with open(os.path.join(root, f'{name_without_json}.yml'), 'w') as yaml_file:
                        yaml.dump(tmp_dict, yaml_file)
            elif name.endswith('.yml') or name.endswith('.yaml'):
                with open(os.path.join(root, name)) as yaml_file:
                    tmp_dict = yaml.load(yaml_file, Loader=SafeLoader)
                if args.convert:
                    name_without_yaml = name[:name.rfind('.')]
                    with open(os.path.join(root, f'{name_without_yaml}.json'), 'w') as json_file:
                        json.dump(tmp_dict, json_file, indent=2)
            if tmp_dict is None:
                continue
            if 'Enums' in tmp_dict:
                json_data['Enums'].extend(tmp_dict['Enums'])
            if 'Groups' in tmp_dict:
                json_data['Groups'].extend(tmp_dict['Groups'])
            if 'Structs' in tmp_dict:
                json_data['Structs'].extend(tmp_dict['Structs'])
            if 'Datapoints' in tmp_dict:
                json_data['Datapoints'].extend(tmp_dict['Datapoints'])
            if 'Processors' in tmp_dict:
                json_data['Processors'].extend(tmp_dict['Processors'])
            if 'Types' in tmp_dict:
                json_data['Types'].extend(tmp_dict['Types'])
    return json_data


def load_validated_model(model_dir, schema_dir):
    """Load, schema-validate, and semantically validate a model directory."""
    with open(f'{schema_dir}/schema.json') as f:
        datalayer_schema = json.load(f)

    json_data = read_model_files(SimpleNamespace(model_dir=model_dir, convert=False))
    validate_json(json_data, datalayer_schema)

    enums = enum_validator(json_data['Enums'])
    types = type_validator(json_data['Types'])
    groups = group_validator(json_data['Groups'])
    structs, struct_names = struct_validator(json_data['Structs'])
    data_points = data_point_validator(json_data['Datapoints'], struct_names, enums, types)
    processors = processor_validator(json_data['Processors'], data_points)
    return {'enums': enums, 'types': types, 'groups': groups, 'structs': structs, 'data_points': data_points, 'processors': processors}


def main(template_file_name, template_formatter_file_name, python_binding_file_name):
    """
    Execute all work for parsing and validating.

    :param template_formatter_file_name: defines the template for the fmt formatter
    :param template_file_name: defines the file that shall be rendered.
    :param python_binding_file_name: defines the python binding template
    """
    args = get_args()
    file_loader = FileSystemLoader(f'{args.template_dir}')
    env = Environment(loader=file_loader)

    model = load_validated_model(args.model_dir, args.schema_dir)
    enums = model['enums']
    types = model['types']
    groups = model['groups']
    structs = model['structs']
    data_points = model['data_points']
    processors = model['processors']

    group_data_points_mapping = create_group_data_point_dict(data_points)
    perfect_hash_data = create_perfect_hash(data_points, groups)
    model_metadata = create_model_metadata(enums, groups, structs, data_points, types)

    if args.check:
        report = {
            'valid': True,
            'groups': len(groups),
            'datapoints': len(data_points),
            'structs': len(structs),
            'enums': len(enums),
            'types': len(types),
            'processors': len(processors),
        }
        print(json.dumps(report, sort_keys=True))
        return

    for path in [f'{args.out_dir}{GENERATED_FOLDER}',
                 f'{args.out_dir}{DOC_FOLDER}',
                 f'{args.out_dir}{PYTHON_FOLDER}']:
        if not os.path.exists(path):
            os.makedirs(path)

    with open(f'{args.out_dir}{DOC_FOLDER}/model-metadata.json', 'w', encoding='utf-8') as f:
        json.dump(model_metadata, f, indent=2)

    template = env.get_template(template_file_name)
    output = template.render(enums=enums, groups=groups, structs=structs, data_points=data_points,
                             group_data_points_mapping=group_data_points_mapping, perfect_hash=perfect_hash_data,
                             prefix_map=PREFIX_MAP, types=types, processors=processors,
                             model_fingerprint=int(model_metadata['modelFingerprint']['value'], 16))
    with open(f'{args.out_dir}{GENERATED_FOLDER}/datalayer.h', 'w') as f:
        f.write(output)

    template = env.get_template(template_formatter_file_name)
    struct_types = dict()
    for struct in structs:
        struct_types[struct['name']] = struct['parameter']
    output = template.render(structs=structs, data_points=data_points, struct_types=struct_types, enums=enums)
    with open(f'{args.out_dir}{GENERATED_FOLDER}/formatter.h', 'w') as f:
        f.write(output)

    template = env.get_template(python_binding_file_name)
    output = template.render(enums=enums, groups=groups, structs=structs, data_points=data_points,
                             group_data_points_mapping=group_data_points_mapping, prefix_map=PREFIX_MAP,
                             module_name=args.module_name)
    with open(f'{args.out_dir}{PYTHON_FOLDER}/pythonBinding.cpp', 'w') as f:
        f.write(output)

    template = env.get_template('rtt.py.jinja2')
    rtt_datapoints = create_rtt_datapoints(data_points, groups, enums, types)
    output = template.render(rtt_datapoints=rtt_datapoints,
                             debug_datapoints=[data_point for data_point in rtt_datapoints
                                               if data_point['access'] != 'WRITE_ONLY'])
    with open(f'{args.out_dir}{PYTHON_FOLDER}/rtt.py', 'w', encoding='utf-8') as f:
        f.write(output)

    template = env.get_template('modelTests.h.jinja2')
    output = template.render(metadata=model_metadata)
    generated_test_directory = f'{args.out_dir}/generated/test'
    if not os.path.exists(generated_test_directory):
        os.makedirs(generated_test_directory)
    with open(f'{generated_test_directory}/modelTests.h', 'w', encoding='utf-8') as f:
        f.write(output)

    generate_uml(enums=enums, structs=structs, datapoints=data_points, types=types,
                 out_dir=f'{args.out_dir}/{DOC_FOLDER}')

    group_ids = dict()
    for group in groups:
        group_ids[group['name']] = int(group['baseId'], 0)

    generate_overview(path=f'{args.out_dir}/{DOC_FOLDER}', group_ids=group_ids, data_point_id=data_points)


if __name__ == '__main__':
    main('datalayer.h.jinja2', 'customFormatter.h.jinja2', 'pythonBinding.jinja2')
