"""Tests for model compatibility reporting."""
import os
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/generator')))
from compatibilityReport import create_compatibility_report


def catalog(data_points):
    """Build a minimal metadata catalog for compatibility tests."""
    return {
        'summary': {'datapoints': len(data_points)},
        'groups': [{'name': 'Default', 'baseId': 0x4000}],
        'datapoints': data_points,
    }


def datapoint(name='value', identifier=0x4001, access='READ_WRITE', data_type='int32_t', legacy_ids=None):
    """Build a minimal datapoint metadata record."""
    return {
        'qualifiedName': name, 'globalId': identifier, 'legacyGlobalIds': legacy_ids or [],
        'access': access, 'type': data_type, 'version': '1.0.0',
    }


class TestCompatibilityReport(unittest.TestCase):
    """Verify breaking and compatible model changes are classified."""

    def test_legacy_id_retains_compatibility_after_id_change(self):
        """Accept an ID change when the previous ID remains a legacy identifier."""
        report = create_compatibility_report(
            catalog([datapoint()]), catalog([datapoint(identifier=0x4002, legacy_ids=[0x4001])]))

        self.assertTrue(report['compatible'])
        self.assertEqual(report['breakingChanges'], [])

    def test_type_and_write_access_changes_are_breaking(self):
        """Classify changed types and removed write access as breaking changes."""
        report = create_compatibility_report(
            catalog([datapoint()]), catalog([datapoint(access='READ_ONLY', data_type='float')]))

        self.assertFalse(report['compatible'])
        self.assertEqual(
            {change['kind'] for change in report['breakingChanges']},
            {'datapointTypeChanged', 'datapointWriteAccessRemoved'},
        )


if __name__ == '__main__':
    unittest.main()
