"""Tests for the local Flask model editor API."""
import json
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/')))
from generator.model_editor import create_app


def valid_model():
    """Return the smallest complete model accepted by the generator."""
    return {
        'Enums': [],
        'Groups': [{'name': 'Main', 'persistence': 'None', 'baseId': '0x4000', 'version': '1.0.0'}],
        'Structs': [],
        'Datapoints': [{'name': 'temperature', 'group': 'Main', 'id': 0, 'type': 'int32_t',
                        'access': 'READ_WRITE', 'version': '1.0.0'}],
        'Types': [],
    }


class TestModelEditor(unittest.TestCase):
    """Exercise model loading, validation, and persistence through Flask."""

    def setUp(self):
        """Create an isolated model directory and test client."""
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.model_directory = self.temporary_directory.name
        for section, value in valid_model().items():
            filename = f'{section.lower()}.json'
            with open(os.path.join(self.model_directory, filename), 'w', encoding='utf-8') as file:
                json.dump({section: value}, file)
        schema_directory = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/generator'))
        self.client = create_app(self.model_directory, schema_directory).test_client()

    def tearDown(self):
        """Remove the temporary model directory."""
        self.temporary_directory.cleanup()

    def test_get_model_returns_editor_metadata(self):
        """The API exposes merged model data and form choices."""
        response = self.client.get('/api/model')
        data = response.get_json()
        self.assertEqual(response.status_code, 200)
        self.assertTrue(data['validation']['valid'])
        self.assertEqual(data['metadata']['groups'], ['Main'])
        self.assertEqual(data['metadata']['nextIds']['Main'], 1)

    def test_invalid_model_is_not_written(self):
        """Saving an invalid model reports the error without modifying source files."""
        model = valid_model()
        model['Datapoints'][0].pop('version')
        response = self.client.put('/api/model', json={'model': model})
        self.assertEqual(response.status_code, 400)
        with open(os.path.join(self.model_directory, 'datapoints.json'), encoding='utf-8') as file:
            self.assertEqual(json.load(file)['Datapoints'][0]['version'], '1.0.0')

    def test_valid_model_is_written_to_canonical_files(self):
        """Saving a valid model persists its sections as formatted JSON files."""
        model = valid_model()
        model['Datapoints'].append({'name': 'pressure', 'group': 'Main', 'id': 1, 'type': 'int32_t',
                                    'access': 'READ_ONLY', 'version': '1.0.0'})
        response = self.client.put('/api/model', json={'model': model})
        self.assertEqual(response.status_code, 200)
        with open(os.path.join(self.model_directory, 'datapoints.json'), encoding='utf-8') as file:
            self.assertEqual(len(json.load(file)['Datapoints']), 2)

    def test_rejects_layouts_that_would_duplicate_model_sections(self):
        """YAML and custom JSON inputs are not safe beside canonical JSON output."""
        with open(os.path.join(self.model_directory, 'legacy.yml'), 'w', encoding='utf-8') as file:
            file.write('Groups: []\n')
        schema_directory = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/generator'))
        with self.assertRaisesRegex(ValueError, 'canonical JSON files only'):
            create_app(self.model_directory, schema_directory)


if __name__ == '__main__':
    unittest.main()
