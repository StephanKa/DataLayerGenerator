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
        test_data = [{'name': 'Temperature', 'parameter': [{'value': 'float'}, {'raw': 'uint32_t'}]}]
        generator.struct_validator(test_data)

    def test_duplicate_struct(self):
        """Test duplicate names for struct."""
        test_data = [{'name': 'Temperature', 'parameter': [{'value3': 'float'}, {'raw1': 'uint32_t'}]},
                     {'name': 'Temperature', 'parameter': [{'value4': 'float'}, {'raw2': 'uint32_t'}]}]
        self.assertRaises(generator.StructException, generator.struct_validator, test_data)

    def test_type_undefined(self):
        """Test undefined type."""
        test_data = [{'name': 'Temperature', 'parameter': [{'name': 'value', 'type': 'uint'}]}]
        self.assertRaises(generator.StructException, generator.struct_validator, test_data)

    def test_base_types(self):
        """Test all supported base types."""
        for base_type in generator.BASE_TYPES:
            test_data = [{'name': 'Temperature', 'parameter': [{'value': base_type}]}]
            generator.struct_validator(test_data)

    def test_without_parameters(self):
        """Test struct without any parameters defined."""
        test_data = [{'name': 'Temperature', 'parameter': []}]
        self.assertRaises(generator.StructException, generator.struct_validator, test_data)

    def test_same_name(self):
        """Test same name definitions."""
        test_data = [{'name': 'Temperature', 'parameter': [{'raw': 'uint32_t'}, {'raw': 'uint32_t'}]}]
        self.assertRaises(generator.StructException, generator.struct_validator, test_data)

    def test_struct_in_struct(self):
        """Test that a struct can use another struct from the same list as a parameter type."""
        test_data = [
            {'name': 'Temperature', 'parameter': [{'value': 'float'}, {'raw': 'uint32_t'}]},
            {'name': 'Environment', 'parameter': [{'internal': 'Temperature'}, {'external': 'Temperature'}]}
        ]
        generator.struct_validator(test_data)

    def test_parameters_sorted_alphabetically(self):
        """Test that parameters are sorted alphabetically in the validated output."""
        test_data = [{'name': 'Sensor', 'parameter': [{'zValue': 'float'}, {'aRaw': 'uint32_t'}]}]
        result, _ = generator.struct_validator(test_data)
        names = [p.name for p in result[0]['parameter']]
        self.assertEqual(names, sorted(names))

    def test_empty_struct_list_returns_empty(self):
        """Test that an empty struct list returns empty data and empty name dict."""
        result, names = generator.struct_validator([])
        self.assertEqual(result, [])
        self.assertEqual(names, {})

    def test_returns_struct_name_in_dict(self):
        """Test that the second return value contains the validated struct name as a key."""
        test_data = [{'name': 'Temperature', 'parameter': [{'value': 'float'}]}]
        _, names = generator.struct_validator(test_data)
        self.assertIn('Temperature', names)

    def test_single_parameter_struct_valid(self):
        """Test that a struct with a single parameter is valid."""
        test_data = [{'name': 'Simple', 'parameter': [{'flag': 'bool'}]}]
        generator.struct_validator(test_data)

    def test_unknown_struct_parameter_type_raises(self):
        """Test that referencing an unknown struct type in a parameter raises StructException."""
        test_data = [{'name': 'Broken', 'parameter': [{'value': 'NonExistentStruct'}]}]
        self.assertRaises(generator.StructException, generator.struct_validator, test_data)


if __name__ == '__main__':
    unittest.main()
