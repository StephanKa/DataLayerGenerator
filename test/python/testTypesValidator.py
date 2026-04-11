"""Test suite to test the struct validator."""
import sys
import os
import unittest
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/')))
import generator


class TestTypesValidator(unittest.TestCase):
    """Test suite to test the struct validator."""

    def test_correct_type(self):
        """Test correct type definition."""
        test_data = [{'name': 'Temperature', 'type': 'uint32_t'}]
        generator.type_validator(test_data)

    def test_type_undefined(self):
        """Test undefined type."""
        test_data = [{'name': 'Temperature', 'type': 'uint'}]
        self.assertRaises(generator.TypeException, generator.type_validator, test_data)

    def test_type_min(self):
        """Test type minimum definitions."""
        test_data = [{'name': 'Temperature', 'type': 'uint32_t', 'min': 12}]
        generator.type_validator(test_data)

    def test_type_max(self):
        """Test type maximum definitions."""
        test_data = [{'name': 'Temperature', 'type': 'uint32_t', 'max': 12}]
        generator.type_validator(test_data)

    def test_type_min_max(self):
        """Test type minimum and maximum definitions."""
        test_data = [{'name': 'Temperature', 'type': 'uint32_t', 'max': 12, 'min': 2}]
        generator.type_validator(test_data)

    def test_type_min_max_wrong(self):
        """Test type minimum and maximum definitions where min is larger than max."""
        test_data = [{'name': 'Temperature', 'type': 'uint32_t', 'max': 12, 'min': 22}]
        self.assertRaises(generator.TypeException, generator.type_validator, test_data)

    def test_type_min_max_equal(self):
        """Test type minimum and maximum definitions where min and max are equal."""
        test_data = [{'name': 'Temperature', 'type': 'uint32_t', 'max': 12, 'min': 12}]
        self.assertRaises(generator.TypeException, generator.type_validator, test_data)

    def test_base_types(self):
        """Test all supported base types."""
        for base_type in generator.BASE_TYPES:
            test_data = [{'name': 'Temperature', 'type': base_type}]
            generator.type_validator(test_data)

    def test_same_name(self):
        """Test same name definitions."""
        test_data = [{'name': 'Temperature', 'type': 'uint32_t'}, {'name': 'Temperature', 'type': 'uint16_t'}]
        self.assertRaises(generator.TypeException, generator.type_validator, test_data)

    def test_empty_type_list(self):
        """Test that an empty list is valid and returned unchanged."""
        result = generator.type_validator([])
        self.assertEqual(result, [])

    def test_multiple_valid_types(self):
        """Test that multiple distinct types with different base types are all valid."""
        test_data = [
            {'name': 'TypeA', 'type': 'uint32_t', 'min': 0, 'max': 100},
            {'name': 'TypeB', 'type': 'float'}
        ]
        generator.type_validator(test_data)

    def test_float_min_max_valid(self):
        """Test that float min/max values are accepted."""
        test_data = [{'name': 'FloatRange', 'type': 'float', 'min': -1.5, 'max': 1.5}]
        generator.type_validator(test_data)

    def test_negative_range_valid(self):
        """Test that a negative min and negative max (min < max) is valid."""
        test_data = [{'name': 'NegRange', 'type': 'int32_t', 'min': -100, 'max': -10}]
        generator.type_validator(test_data)

    def test_missing_type_key_raises(self):
        """Test that a type definition missing the 'type' field raises TypeException."""
        test_data = [{'name': 'NoType'}]
        self.assertRaises(generator.TypeException, generator.type_validator, test_data)

    def test_min_max_defaulted_to_none_when_absent(self):
        """Test that min and max are set to None when not provided."""
        test_data = [{'name': 'T', 'type': 'uint32_t'}]
        result = generator.type_validator(test_data)
        self.assertIsNone(result[0]['min'])
        self.assertIsNone(result[0]['max'])


if __name__ == '__main__':
    unittest.main()
