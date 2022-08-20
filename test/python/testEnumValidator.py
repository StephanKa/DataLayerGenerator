"""Test suite to test the enum validator."""
import sys
import os
import unittest
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/')))
import generator


class TestEnumValidator(unittest.TestCase):
    """Test suite to test the enum validator."""

    def test_correct_enum(self):
        """Test correct enum definition."""
        test_data = [{'name': 'Status', 'type': 'uint32_t', 'values': ['Booting', 'Starting', 'Finished'], 'autoId': True}]
        generator.enum_validator(test_data)

    def test_duplicate_enum(self):
        """Test duplicate names."""
        test_data = [{'name': 'Status', 'type': 'uint32_t', 'values': ['Booting']}, {'name': 'Status', 'type': 'uint32_t', 'values': ['Booting']}]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)

    def test_type_undefined(self):
        """Test undefined type."""
        test_data = [{'name': 'Status', 'type': 'uint32', 'values': ['Booting']}]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)

    def test_base_types(self):
        """Test all base types."""
        for base_type in generator.BASE_TYPES:
            test_data = [{'name': 'Status', 'type': base_type, 'values': ['Booting']}]
            generator.enum_validator(test_data)

    def test_auto_id_with_value(self):
        """Test auto id with any values."""
        test_data = [{'name': 'Status', 'type': 'uint32_t', 'values': ['Booting'], 'autoId': True}]
        generator.enum_validator(test_data)

    def test_auto_id_without_values(self):
        """Test auto id without any values."""
        test_data = [{'name': 'Status', 'type': 'uint32', 'values': [], 'autoId': True}]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)

    def test_auto_id_without_string_value(self):
        """Test auto id without a string as name."""
        test_data = [{'name': 'Status', 'type': 'uint32', 'values': [1], 'autoId': True}]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)


if __name__ == '__main__':
    unittest.main()
