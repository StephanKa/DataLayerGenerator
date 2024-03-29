"""This generator will validate json defined JSON as model via a schema and generate according C++."""
import json
import os
import sys
import jsonschema
import argparse
import yaml
from yaml.loader import SafeLoader
from jinja2 import Environment, FileSystemLoader
from constants import PREFIX_MAP
from validators import enum_validator, struct_validator, group_validator, data_point_validator, type_validator
from umlGenerator import generate_uml
from overviewGenerator import generate_overview

GENERATED_FOLDER = '/generated/include'
DOC_FOLDER = '/generated/doc'
PYTHON_FOLDER = '/generated/python'


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
    json_data = {'Enums': [], 'Groups': [], 'Structs': [], 'Datapoints': [], 'Types': []}
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

            if json_data is None:
                json_data = tmp_dict
                continue
            if 'Enums' in tmp_dict:
                json_data['Enums'].extend(tmp_dict['Enums'])
            if 'Groups' in tmp_dict:
                json_data['Groups'].extend(tmp_dict['Groups'])
            if 'Structs' in tmp_dict:
                json_data['Structs'].extend(tmp_dict['Structs'])
            if 'Datapoints' in tmp_dict:
                json_data['Datapoints'].extend(tmp_dict['Datapoints'])
            if 'Types' in tmp_dict:
                json_data['Types'].extend(tmp_dict['Types'])
    return json_data


def main(template_file_name, template_formatter_file_name, python_binding_file_name):
    """
    Execute all work for parsing and validating.

    :param template_formatter_file_name: defines the template for the fmt formatter
    :param template_file_name: defines the file that shall be rendered.
    :param python_binding_file_name: defines the python binding template
    """
    args = get_args()
    with open(f'{args.schema_dir}/schema.json') as f:
        datalayer_schema = json.load(f)

    file_loader = FileSystemLoader(f'{args.template_dir}')
    env = Environment(loader=file_loader)

    json_data = read_model_files(args)

    # validate json scheme
    validate_json(json_data, datalayer_schema)

    # validate all defined sections

    enums = enum_validator(json_data['Enums'])
    types = type_validator(json_data['Types'])
    groups = group_validator(json_data['Groups'])
    structs, struct_names = struct_validator(json_data['Structs'])
    data_points = data_point_validator(json_data['Datapoints'], struct_names, enums, types)

    group_data_points_mapping = create_group_data_point_dict(data_points)

    for path in [f'{args.out_dir}{GENERATED_FOLDER}',
                 f'{args.out_dir}{DOC_FOLDER}',
                 f'{args.out_dir}{PYTHON_FOLDER}']:
        if not os.path.exists(path):
            os.makedirs(path)

    template = env.get_template(template_file_name)
    output = template.render(enums=enums, groups=groups, structs=structs, data_points=data_points,
                             group_data_points_mapping=group_data_points_mapping, prefix_map=PREFIX_MAP, types=types)
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
                             group_data_points_mapping=group_data_points_mapping, prefix_map=PREFIX_MAP)
    with open(f'{args.out_dir}{PYTHON_FOLDER}/pythonBinding.cpp', 'w') as f:
        f.write(output)

    generate_uml(enums=json_data['Enums'], structs=json_data['Structs'], datapoint=json_data['Datapoints'],
                 out_dir=f'{args.out_dir}/{DOC_FOLDER}')

    group_ids = dict()
    for group in groups:
        group_ids[group['name']] = int(group['baseId'], 0)

    generate_overview(path=f'{args.out_dir}/{DOC_FOLDER}', group_ids=group_ids, data_point_id=json_data['Datapoints'])


if __name__ == '__main__':
    main('datalayer.h.jinja2', 'customFormatter.h.jinja2', 'pythonBinding.jinja2')
