"""Tests for machine-readable model metadata generation."""
import os
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/generator')))
from modelMetadata import create_model_metadata
from validators import Version


class TestModelMetadata(unittest.TestCase):
    """Verify metadata exposes stable model identifiers and summary data."""

    def test_catalog_includes_global_and_legacy_datapoint_ids(self):
        """Catalog current and legacy global IDs with qualified datapoint names."""
        metadata = create_model_metadata(
            enums=[],
            groups=[{
                'name': 'Configuration', 'baseId': '0x4000', 'version': Version('1.2.3'),
                'persistence': 'None', 'allowUpgrade': 'false', 'maxPayloadBytes': None, 'description': None,
            }],
            structs=[],
            data_points=[{
                'name': 'temperature', 'namespace': 'Sensors', 'group': 'Configuration', 'id': 3,
                'renamedFrom': [1], 'type': 'int32_t', 'access': 'READ_WRITE', 'arraySize': 0,
                'version': Version('2.0.0'), 'allowUpgrade': 'true', 'description': 'Measured temperature',
            }],
            types=[],
        )

        self.assertEqual(metadata['summary']['datapointsByAccess']['READ_WRITE'], 1)
        self.assertEqual(metadata['groups'][0]['baseId'], 0x4000)
        self.assertEqual(metadata['datapoints'], [{
            'name': 'temperature', 'qualifiedName': 'Sensors::temperature', 'group': 'Configuration',
            'id': 3, 'globalId': 0x4003, 'legacyGlobalIds': [0x4001], 'type': 'int32_t',
            'access': 'READ_WRITE', 'arraySize': 0, 'version': '2.0.0', 'allowUpgrade': True,
            'description': 'Measured temperature',
        }])


if __name__ == '__main__':
    unittest.main()
