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
        test_data = [{'name': 'Status', 'type': 'uint32_t', 'values': ['Booting'], 'autoId': True}, {'name': 'Status', 'type': 'uint32_t', 'values': ['Booting'], 'autoId': True}]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)

    def test_type_undefined(self):
        """Test undefined type."""
        test_data = [{'name': 'Status', 'type': 'uint32', 'values': ['Booting']}]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)

    def test_base_types(self):
        """Test all base types."""
        for base_type in generator.BASE_TYPES:
            test_data = [{'name': 'Status', 'type': base_type, 'values': ['Booting'], 'autoId': True}]
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

    def test_missing_autoId(self):
        """Test duplicate names."""
        test_data = [{'name': 'Status', 'type': 'uint32_t', 'values': ['Booting']}]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)

    def test_explicit_values(self):
        """Test enum with explicit key-value pairs (autoId=False) is valid."""
        test_data = [{'name': 'ErrorStates', 'type': 'uint32_t',
                      'values': [{'None': 0}, {'Info': 1}, {'Error': 2}], 'autoId': False}]
        generator.enum_validator(test_data)

    def test_empty_enum_list(self):
        """Test that an empty list is valid and returned unchanged."""
        result = generator.enum_validator([])
        self.assertEqual(result, [])

    def test_whitespace_name_treated_as_duplicate(self):
        """Test that names differing only in surrounding whitespace are duplicates."""
        test_data = [
            {'name': 'Status ', 'type': 'uint32_t', 'values': ['Booting'], 'autoId': True},
            {'name': 'Status', 'type': 'uint32_t', 'values': ['Starting'], 'autoId': True}
        ]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)

    def test_autoId_false_empty_values_valid(self):
        """Test autoId=False with empty values is valid (no values to iterate)."""
        test_data = [{'name': 'Empty', 'type': 'uint32_t', 'values': [], 'autoId': False}]
        generator.enum_validator(test_data)

    def test_multiple_enums_mixed_autoId(self):
        """Test two enums with different autoId modes are both valid."""
        test_data = [
            {'name': 'Status', 'type': 'uint32_t', 'values': ['Booting', 'Running'], 'autoId': True},
            {'name': 'ErrorStates', 'type': 'uint8_t', 'values': [{'None': 0}, {'Error': 1}], 'autoId': False}
        ]
        generator.enum_validator(test_data)

    def test_autoId_false_string_values_raises(self):
        """Test autoId=False with string-only values raises EnumException."""
        test_data = [{'name': 'Status', 'type': 'uint32_t', 'values': ['Booting'], 'autoId': False}]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)

    def test_autoId_true_dict_values_raises(self):
        """Test autoId=True with dict (explicit) values raises EnumException."""
        test_data = [{'name': 'Status', 'type': 'uint32_t', 'values': [{'Booting': 0}], 'autoId': True}]
        self.assertRaises(generator.EnumException, generator.enum_validator, test_data)

    def test_explicit_values_transformed_to_parameter_objects(self):
        """Test that explicit enum values are transformed into Parameter-like objects with name/value."""
        test_data = [{'name': 'Status', 'type': 'uint32_t', 'values': [{'Active': 7}], 'autoId': False}]
        result = generator.enum_validator(test_data)
        param = result[0]['values'][0]
        self.assertEqual(param.name, 'Active')
        self.assertEqual(param.value, 7)


if __name__ == '__main__':
    unittest.main()
