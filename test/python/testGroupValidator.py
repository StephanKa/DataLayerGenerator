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
        test_data = [{'name': 'DefaultGroup', 'persistence': 'None', 'baseId': '0x4000', 'version': '1.0.1'}]
        generator.group_validator(test_data)

    def test_multiple_correct_group(self):
        """Test multiple correct group definitions."""
        test_data = [{'name': 'DefaultGroup', 'persistence': 'None', 'baseId': '0x4000', 'version': '1.0.1'},
                     {'name': 'DefaultGroup1', 'persistence': 'None', 'baseId': '0x4001', 'version': '1.0.1'}]
        generator.group_validator(test_data)

    def test_undefined_persistence(self):
        """Test undefined persistence."""
        test_data = [{'name': 'DefaultGroup', 'persistence': 'Always', 'baseId': '0x4000', 'version': '1.0.1'}]
        self.assertRaises(generator.GroupException, generator.group_validator, test_data)

    def test_same_name(self):
        """Test same name definitions."""
        test_data = [{'name': 'DefaultGroup', 'persistence': 'None', 'baseId': '0x4000', 'version': '1.0.1'},
                     {'name': 'DefaultGroup', 'persistence': 'None', 'baseId': '0x4001', 'version': '1.0.1'}]
        self.assertRaises(generator.GroupException, generator.group_validator, test_data)

    def test_persistence(self):
        """Test all supported persistence types."""
        for persistence in generator.SUPPORTED_PERSISTENCE:
            test_data = [{'name': 'DefaultGroup', 'persistence': persistence, 'baseId': '0x4000', 'version': '1.0.1'}]
            generator.group_validator(test_data)

    def test_duplicate_base_id(self):
        """Test duplicate base id's."""
        test_data = [{'name': 'DefaultGroup1', 'persistence': 'None', 'baseId': '0x4000', 'version': '1.0.1'},
                     {'name': 'DefaultGroup2', 'persistence': 'None', 'baseId': '0x4000', 'version': '1.0.1'}]
        self.assertRaises(generator.GroupException, generator.group_validator, test_data)

    def test_empty_group_list(self):
        """Test that an empty list is valid and returned unchanged."""
        result = generator.group_validator([])
        self.assertEqual(result, [])

    def test_allow_upgrade_true_becomes_string(self):
        """Test that allowUpgrade=True is converted to the string 'true'."""
        test_data = [{'name': 'G', 'persistence': 'None', 'baseId': '0x1000', 'version': '1.0.0',
                      'allowUpgrade': True}]
        result = generator.group_validator(test_data)
        self.assertEqual(result[0]['allowUpgrade'], 'true')

    def test_allow_upgrade_false_becomes_string(self):
        """Test that allowUpgrade=False is converted to the string 'false'."""
        test_data = [{'name': 'G', 'persistence': 'None', 'baseId': '0x1000', 'version': '1.0.0',
                      'allowUpgrade': False}]
        result = generator.group_validator(test_data)
        self.assertEqual(result[0]['allowUpgrade'], 'false')

    def test_missing_allow_upgrade_defaults_false(self):
        """Test that a missing allowUpgrade field defaults to string 'false'."""
        test_data = [{'name': 'G', 'persistence': 'None', 'baseId': '0x1000', 'version': '1.0.0'}]
        result = generator.group_validator(test_data)
        self.assertEqual(result[0]['allowUpgrade'], 'false')

    def test_missing_description_defaults_none(self):
        """Test that a missing description field is injected as None."""
        test_data = [{'name': 'G', 'persistence': 'None', 'baseId': '0x1000', 'version': '1.0.0'}]
        result = generator.group_validator(test_data)
        self.assertIsNone(result[0]['description'])

    def test_version_parsed_to_object(self):
        """Test that the version string is replaced by a Version-like object."""
        test_data = [{'name': 'G', 'persistence': 'None', 'baseId': '0x1000', 'version': '2.3.4'}]
        result = generator.group_validator(test_data)
        self.assertEqual(str(result[0]['version']), '2.3.4')

    def test_invalid_version_format_raises(self):
        """Test that a version string with fewer than 3 parts raises ValueError."""
        test_data = [{'name': 'G', 'persistence': 'None', 'baseId': '0x1000', 'version': '1.0'}]
        self.assertRaises(ValueError, generator.group_validator, test_data)


if __name__ == '__main__':
    unittest.main()
