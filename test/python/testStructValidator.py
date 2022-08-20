"""Test suite to test the struct validator."""
import sys
import os
import unittest
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/')))
import generator


class TestStructValidator(unittest.TestCase):
    """Test suite to test the struct validator."""

    def test_correct_struct(self):
        """Test correct struct definition."""
        test_data = [{'name': 'Temperature', 'parameter': [{'name': 'value', 'type': 'float'}, {'name': 'raw', 'type': 'uint32_t'}]}]
        generator.struct_validator(test_data)

    def test_duplicate_struct(self):
        """Test duplicate names for struct."""
        test_data = [{'name': 'Temperature', 'parameter': [{'name': 'value3', 'type': 'float'}, {'name': 'raw1', 'type': 'uint32_t'}]},
                     {'name': 'Temperature', 'parameter': [{'name': 'value4', 'type': 'float'}, {'name': 'raw2', 'type': 'uint32_t'}]}]
        self.assertRaises(generator.StructException, generator.struct_validator, test_data)

    def test_type_undefined(self):
        """Test undefined type."""
        test_data = [{'name': 'Temperature', 'parameter': [{'name': 'value', 'type': 'uint'}]}]
        self.assertRaises(generator.StructException, generator.struct_validator, test_data)

    def test_base_types(self):
        """Test all supported base types."""
        for base_type in generator.BASE_TYPES:
            test_data = [{'name': 'Temperature', 'parameter': [{'name': 'value', 'type': base_type}]}]
            generator.struct_validator(test_data)

    def test_without_parameters(self):
        """Test struct without any parameters defined."""
        test_data = [{'name': 'Temperature', 'parameter': []}]
        self.assertRaises(generator.StructException, generator.struct_validator, test_data)

    def test_same_name(self):
        """Test same name definitions."""
        test_data = [{'name': 'Temperature', 'parameter': [{'name': 'raw', 'type': 'uint32_t'}, {'name': 'raw', 'type': 'uint32_t'}]}]
        self.assertRaises(generator.StructException, generator.struct_validator, test_data)


if __name__ == '__main__':
    unittest.main()
