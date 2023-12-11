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


if __name__ == '__main__':
    unittest.main()
