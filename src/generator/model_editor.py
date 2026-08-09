"""Run a local browser UI for authoring DataLayerGenerator models."""
import argparse
import copy
import json
import os
from pathlib import Path
from types import SimpleNamespace

from flask import Flask, jsonify, render_template, request
from jinja2 import Environment, FileSystemLoader
from jsonschema import ValidationError

if __package__:
    from . import generator as generator_module
    from . import validators as validators_module
    from .constants import BASE_ACCESS, BASE_TYPES, PREFIX_MAP, SUPPORTED_PERSISTENCE
    from .generator import create_group_data_point_dict, create_perfect_hash, read_model_files, validate_json
    from .validatorException import DatapointException, EnumException, GroupException, StructException, TypeException
    from .validators import data_point_validator, enum_validator, group_validator, struct_validator, type_validator
else:
    import generator as generator_module
    import validators as validators_module
    from constants import BASE_ACCESS, BASE_TYPES, PREFIX_MAP, SUPPORTED_PERSISTENCE
    from generator import create_group_data_point_dict, create_perfect_hash, read_model_files, validate_json
    from validatorException import DatapointException, EnumException, GroupException, StructException, TypeException
    from validators import data_point_validator, enum_validator, group_validator, struct_validator, type_validator

SECTION_FILENAMES = {
    'Enums': 'enums.json',
    'Groups': 'groups.json',
    'Structs': 'structs.json',
    'Datapoints': 'datapoints.json',
    'Types': 'types.json',
}
SECTIONS = tuple(SECTION_FILENAMES)
BASE_TYPE_SIZES = {
    'bool': 1,
    'uint8_t': 1,
    'int8_t': 1,
    'uint16_t': 2,
    'int16_t': 2,
    'uint32_t': 4,
    'int32_t': 4,
    'float': 4,
    'uint64_t': 8,
    'int64_t': 8,
    'double': 8,
}
MODEL_VALIDATION_ERRORS = (
    DatapointException,
    EnumException,
    GroupException,
    StructException,
    TypeException,
    generator_module.DatapointException,
    validators_module.DatapointException,
    validators_module.EnumException,
    validators_module.GroupException,
    validators_module.StructException,
    validators_module.TypeException,
    ValidationError,
    ValueError,
)


def get_arguments():
    """Return command-line arguments for the local editor."""
    parser = argparse.ArgumentParser(description='Open a local browser UI for a DataLayerGenerator model.')
    parser.add_argument('--model-dir', required=True, help='Directory containing canonical JSON model files.')
    parser.add_argument('--schema-dir', default=Path(__file__).parent,
                        help='Directory containing schema.json (default: this module directory).')
    parser.add_argument('--host', default='127.0.0.1', help='Host interface to bind (default: 127.0.0.1).')
    parser.add_argument('--port', type=int, default=5000, help='HTTP port to bind (default: 5000).')
    return parser.parse_args()


def normalize_model(model):
    """Return a model with every supported section represented by a list."""
    if not isinstance(model, dict):
        raise ValueError('The model must be a JSON object.')
    normalized = {section: model.get(section, []) for section in SECTIONS}
    for section, value in normalized.items():
        if not isinstance(value, list):
            raise ValueError(f'{section} must be an array.')
    return normalized


def validated_sections(model, schema):
    """Return normalized, semantically validated sections for a model."""
    candidate = copy.deepcopy(normalize_model(model))
    validate_json(candidate, schema)
    enums = enum_validator(candidate['Enums'])
    types = type_validator(candidate['Types'])
    groups = group_validator(candidate['Groups'])
    structs, struct_names = struct_validator(candidate['Structs'])
    data_points = data_point_validator(candidate['Datapoints'], struct_names, enums, types)
    return candidate, enums, types, groups, structs, data_points


def validate_model(model, schema):
    """Validate model syntax and the generator's cross-reference rules."""
    try:
        candidate, enums, types, groups, _, _ = validated_sections(model, schema)
    except MODEL_VALIDATION_ERRORS as error:
        return {'valid': False, 'error': str(error)}
    return {
        'valid': True,
        'counts': {
            'groups': len(groups),
            'datapoints': len(candidate['Datapoints']),
            'structs': len(candidate['Structs']),
            'enums': len(enums),
            'types': len(types),
        },
    }


def load_model(model_dir):
    """Load and merge the JSON/YAML files accepted by the code generator."""
    return read_model_files(SimpleNamespace(model_dir=str(model_dir), convert=False))


def validate_model_layout(model_dir):
    """Reject input layouts that canonical JSON output could duplicate on a later load."""
    canonical_files = {model_dir / filename for filename in SECTION_FILENAMES.values()}
    input_files = set()
    for pattern in ('*.json', '*.yaml', '*.yml'):
        input_files.update(model_dir.rglob(pattern))
    unsupported_files = sorted(file for file in input_files if file not in canonical_files)
    if unsupported_files:
        formatted_files = ', '.join(str(file.relative_to(model_dir)) for file in unsupported_files)
        raise ValueError('The model editor supports canonical JSON files only: '
                         f'{", ".join(SECTION_FILENAMES.values())}. Unsupported input: {formatted_files}')


def write_model(model_dir, model):
    """Atomically write the editor's canonical JSON files after validation."""
    for section, filename in SECTION_FILENAMES.items():
        destination = model_dir / filename
        temporary = destination.with_suffix('.json.tmp')
        with temporary.open('w', encoding='utf-8', newline='\n') as file:
            json.dump({section: model[section]}, file, indent=2)
            file.write('\n')
        os.replace(temporary, destination)


def parse_base_id(base_id):
    """Return a numeric group base ID or ``None`` for incomplete editor input."""
    try:
        return int(base_id, 0) if isinstance(base_id, str) else int(base_id)
    except (TypeError, ValueError):
        return None


def payload_size_resolver(model):
    """Return a function that estimates portable payload sizes by model type name."""
    enums = {item['name']: item['type'] for item in model['Enums']}
    aliases = {item['name']: item['type'] for item in model['Types']}
    structs = {item['name']: item['parameter'] for item in model['Structs']}

    def size_of(type_name, visited=frozenset()):
        if type_name in BASE_TYPE_SIZES:
            return BASE_TYPE_SIZES[type_name]
        if type_name in visited:
            return None
        if type_name in enums:
            return size_of(enums[type_name], visited | {type_name})
        if type_name in aliases:
            return size_of(aliases[type_name], visited | {type_name})
        if type_name in structs:
            member_sizes = []
            for parameter in structs[type_name]:
                if not isinstance(parameter, dict) or len(parameter) != 1:
                    return None
                member_size = size_of(next(iter(parameter.values())), visited | {type_name})
                if member_size is None:
                    return None
                member_sizes.append(member_size)
            return sum(member_sizes)
        return None

    return size_of


def model_analysis(model):
    """Return collisions, budgets, references, and migrations for editor display."""
    groups = {group.get('name'): group for group in model['Groups']}
    datapoints_by_group = {name: [] for name in groups}
    relative_ids = {}
    absolute_ids = {}
    migrations = []
    for datapoint in model['Datapoints']:
        group_name = datapoint.get('group')
        datapoints_by_group.setdefault(group_name, []).append(datapoint)
        relative_ids.setdefault((group_name, datapoint.get('id')), []).append(datapoint.get('name', ''))
        base_id = parse_base_id(groups.get(group_name, {}).get('baseId'))
        if base_id is not None and isinstance(datapoint.get('id'), int):
            absolute_ids.setdefault(base_id + datapoint['id'], []).append(datapoint.get('name', ''))
        if datapoint.get('migration'):
            migrations.append({'section': 'Datapoints', 'name': datapoint.get('name', ''),
                               'migration': datapoint['migration'], 'allowUpgrade': bool(datapoint.get('allowUpgrade'))})
    for group in model['Groups']:
        if group.get('migration'):
            migrations.append({'section': 'Groups', 'name': group.get('name', ''),
                               'migration': group['migration'], 'allowUpgrade': bool(group.get('allowUpgrade'))})

    collisions = []
    for (group_name, identifier), names in relative_ids.items():
        if len(names) > 1:
            collisions.append({'kind': 'relative', 'group': group_name, 'id': identifier, 'datapoints': names})
    for identifier, names in absolute_ids.items():
        if len(names) > 1:
            collisions.append({'kind': 'absolute', 'id': identifier, 'datapoints': names})

    size_of = payload_size_resolver(model)
    payload_budgets = []
    id_ranges = []
    for group_name, group in groups.items():
        identifiers = [item.get('id') for item in datapoints_by_group.get(group_name, []) if isinstance(item.get('id'), int)]
        base_id = parse_base_id(group.get('baseId'))
        id_ranges.append({
            'group': group_name,
            'baseId': base_id,
            'start': base_id + min(identifiers) if base_id is not None and identifiers else None,
            'end': base_id + max(identifiers) if base_id is not None and identifiers else None,
        })
        sizes = [size_of(datapoint.get('type')) for datapoint in datapoints_by_group.get(group_name, [])]
        used_bytes = None if any(size is None for size in sizes) else sum(
            size * max(1, datapoint.get('arraySize', 0))
            for datapoint, size in zip(datapoints_by_group.get(group_name, []), sizes))
        maximum = group.get('maxPayloadBytes')
        payload_budgets.append({
            'group': group_name,
            'usedBytes': used_bytes,
            'maxBytes': maximum,
            'remainingBytes': maximum - used_bytes if maximum is not None and used_bytes is not None else None,
            'overBudget': maximum is not None and used_bytes is not None and used_bytes > maximum,
        })

    type_names = [item['name'] for item in model['Structs'] + model['Enums'] + model['Types']]
    type_references = []
    for type_name in type_names:
        type_references.append({
            'name': type_name,
            'datapoints': [item.get('name', '') for item in model['Datapoints'] if item.get('type') == type_name],
            'structs': [item.get('name', '') for item in model['Structs'] if any(
                isinstance(parameter, dict) and type_name in parameter.values() for parameter in item.get('parameter', []))],
        })
    return {
        'collisions': collisions,
        'idRanges': id_ranges,
        'payloadBudgets': payload_budgets,
        'references': {
            'groups': [{'name': name, 'datapoints': [item.get('name', '') for item in datapoints]}
                       for name, datapoints in datapoints_by_group.items()],
            'types': type_references,
        },
        'migrations': migrations,
    }


def model_metadata(model):
    """Return editor choices and derived values for the current model."""
    type_names = BASE_TYPES + [item['name'] for item in model['Structs']] + [item['name'] for item in model['Enums']]
    type_names.extend(item['name'] for item in model['Types'])
    used_ids = {group['name']: set() for group in model['Groups']}
    for datapoint in model['Datapoints']:
        used_ids.setdefault(datapoint.get('group'), set()).add(datapoint.get('id'))
    next_ids = {}
    for group, ids in used_ids.items():
        next_id = 0
        while next_id in ids:
            next_id += 1
        next_ids[group] = next_id
    return {
        'baseTypes': BASE_TYPES,
        'accessModes': BASE_ACCESS,
        'persistenceModes': SUPPORTED_PERSISTENCE,
        'groups': [group['name'] for group in model['Groups']],
        'types': sorted(type_names),
        'nextIds': next_ids,
        'analysis': model_analysis(model),
    }


def render_header_preview(model, schema):
    """Render the generated C++ header in memory without writing model output."""
    _, enums, types, groups, structs, data_points = validated_sections(model, schema)
    template_directory = Path(__file__).parent.parent / 'template'
    template = Environment(loader=FileSystemLoader(template_directory)).get_template('datalayer.h.jinja2')
    return template.render(
        enums=enums,
        groups=groups,
        structs=structs,
        data_points=data_points,
        group_data_points_mapping=create_group_data_point_dict(data_points),
        perfect_hash=create_perfect_hash(data_points, groups),
        prefix_map=PREFIX_MAP,
        types=types,
    )


def create_app(model_dir, schema_dir):
    """Create the local Flask application for a single model directory."""
    app = Flask(__name__)
    app.config['MODEL_DIR'] = Path(model_dir).resolve()
    app.config['SCHEMA_PATH'] = Path(schema_dir).resolve() / 'schema.json'
    if not app.config['MODEL_DIR'].is_dir():
        raise ValueError(f'Model directory does not exist: {app.config["MODEL_DIR"]}')
    validate_model_layout(app.config['MODEL_DIR'])
    with app.config['SCHEMA_PATH'].open(encoding='utf-8') as file:
        app.config['SCHEMA'] = json.load(file)

    @app.get('/')
    def index():
        """Render the model editor."""
        asset_version = max(
            (Path(app.static_folder) / 'model_editor.css').stat().st_mtime_ns,
            (Path(app.static_folder) / 'model_editor.js').stat().st_mtime_ns,
        )
        return render_template('model_editor.html', model_directory=str(app.config['MODEL_DIR']),
                               asset_version=asset_version)

    @app.get('/api/model')
    def get_model():
        """Return the merged model and editor metadata."""
        model = normalize_model(load_model(app.config['MODEL_DIR']))
        return jsonify({'model': model, 'metadata': model_metadata(model),
                        'validation': validate_model(model, app.config['SCHEMA'])})

    @app.post('/api/validate')
    def validate():
        """Validate a client-side model without writing files."""
        payload = request.get_json(silent=True) or {}
        try:
            model = normalize_model(payload.get('model'))
        except ValueError as error:
            return jsonify({'valid': False, 'error': str(error)}), 400
        return jsonify(validate_model(model, app.config['SCHEMA']))

    @app.post('/api/analyze')
    def analyze():
        """Return derived editor analysis for a client-side model without writing files."""
        payload = request.get_json(silent=True) or {}
        try:
            model = normalize_model(payload.get('model'))
        except ValueError as error:
            return jsonify({'error': str(error)}), 400
        return jsonify({'metadata': model_metadata(model), 'validation': validate_model(model, app.config['SCHEMA'])})

    @app.post('/api/preview')
    def preview():
        """Render a generated header preview after validating a client-side model."""
        payload = request.get_json(silent=True) or {}
        try:
            model = normalize_model(payload.get('model'))
        except ValueError as error:
            return jsonify({'valid': False, 'error': str(error)}), 400
        validation = validate_model(model, app.config['SCHEMA'])
        if not validation['valid']:
            return jsonify(validation), 400
        try:
            header = render_header_preview(model, app.config['SCHEMA'])
        except MODEL_VALIDATION_ERRORS as error:
            return jsonify({'valid': False, 'error': str(error)}), 400
        return jsonify({'valid': True, 'header': header})

    @app.put('/api/model')
    def save_model():
        """Validate and persist a complete model submitted by the editor."""
        payload = request.get_json(silent=True) or {}
        try:
            model = normalize_model(payload.get('model'))
        except ValueError as error:
            return jsonify({'valid': False, 'error': str(error)}), 400
        validation = validate_model(model, app.config['SCHEMA'])
        if not validation['valid']:
            return jsonify(validation), 400
        write_model(app.config['MODEL_DIR'], model)
        return jsonify({'valid': True, 'metadata': model_metadata(model), 'validation': validation})

    return app


def main():
    """Start the local model editor server."""
    arguments = get_arguments()
    app = create_app(arguments.model_dir, arguments.schema_dir)
    app.run(host=arguments.host, port=arguments.port, debug=False)


if __name__ == '__main__':
    main()
