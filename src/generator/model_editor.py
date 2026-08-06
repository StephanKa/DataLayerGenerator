"""Run a local browser UI for authoring DataLayerGenerator models."""
import argparse
import copy
import json
import os
from pathlib import Path
from types import SimpleNamespace

from flask import Flask, jsonify, render_template, request
from jsonschema import ValidationError

if __package__:
    from .constants import BASE_ACCESS, BASE_TYPES, SUPPORTED_PERSISTENCE
    from .generator import read_model_files, validate_json
    from .validatorException import DatapointException, EnumException, GroupException, StructException, TypeException
    from .validators import data_point_validator, enum_validator, group_validator, struct_validator, type_validator
else:
    from constants import BASE_ACCESS, BASE_TYPES, SUPPORTED_PERSISTENCE
    from generator import read_model_files, validate_json
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


def validate_model(model, schema):
    """Validate model syntax and the generator's cross-reference rules."""
    candidate = copy.deepcopy(normalize_model(model))
    try:
        validate_json(candidate, schema)
        enums = enum_validator(candidate['Enums'])
        types = type_validator(candidate['Types'])
        groups = group_validator(candidate['Groups'])
        _, struct_names = struct_validator(candidate['Structs'])
        data_point_validator(candidate['Datapoints'], struct_names, enums, types)
    except (DatapointException, EnumException, GroupException, StructException, TypeException, ValidationError, ValueError) as error:
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
    }


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
        return render_template('model_editor.html', model_directory=str(app.config['MODEL_DIR']))

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
