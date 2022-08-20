"""Test suite to test the group validator."""
import sys
import os
import unittest
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/')))
import generator


class TestGroupValidator(unittest.TestCase):
    """Test suite to test the group validator."""

    def test_correct_struct(self):
        """Test correct group definition."""
        test_data = [{'name': 'DefaultGroup', 'persistence': 'None', 'baseId': '0x4000', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        generator.group_validator(test_data)

    def test_multiple_correct_group(self):
        """Test multiple correct group definitions."""
        test_data = [{'name': 'DefaultGroup', 'persistence': 'None', 'baseId': '0x4000', 'version': {'major': 1, 'minor': 0, 'build': 1}},
                     {'name': 'DefaultGroup1', 'persistence': 'None', 'baseId': '0x4001', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        generator.group_validator(test_data)

    def test_undefined_persistence(self):
        """Test undefined persistence."""
        test_data = [{'name': 'DefaultGroup', 'persistence': 'Always', 'baseId': '0x4000', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        self.assertRaises(generator.GroupException, generator.group_validator, test_data)

    def test_same_name(self):
        """Test same name definitions."""
        test_data = [{'name': 'DefaultGroup', 'persistence': 'None', 'baseId': '0x4000', 'version': {'major': 1, 'minor': 0, 'build': 1}},
                     {'name': 'DefaultGroup', 'persistence': 'None', 'baseId': '0x4001', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        self.assertRaises(generator.GroupException, generator.group_validator, test_data)

    def test_persistence(self):
        """Test all supported persistence types."""
        for persistence in generator.SUPPORTED_PERSISTENCE:
            test_data = [{'name': 'DefaultGroup', 'persistence': persistence, 'baseId': '0x4000', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
            generator.group_validator(test_data)

    def test_duplicate_base_id(self):
        """Test duplicate base id's."""
        test_data = [{'name': 'DefaultGroup1', 'persistence': 'None', 'baseId': '0x4000', 'version': {'major': 1, 'minor': 0, 'build': 1}},
                     {'name': 'DefaultGroup2', 'persistence': 'None', 'baseId': '0x4000', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        self.assertRaises(generator.GroupException, generator.group_validator, test_data)


if __name__ == '__main__':
    unittest.main()
