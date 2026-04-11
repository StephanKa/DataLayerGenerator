"""Test suite for Parameter, Version, ParameterList and validate_data_default_struct helpers."""
import sys
import os
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/generator')))
from validators import Parameter, Version, ParameterList, validate_data_default_struct


class TestParameter(unittest.TestCase):
    """Tests for the Parameter helper class."""

    def test_simple_string_value_extracts_name_and_value(self):
        """Test that a basic string value is split into name and value."""
        p = Parameter({'myField': 'uint32_t'})
        self.assertEqual(p.name, 'myField')
        self.assertEqual(p.value, 'uint32_t')

    def test_integer_value(self):
        """Test Parameter with an integer value."""
        p = Parameter({'count': 42})
        self.assertEqual(p.name, 'count')
        self.assertEqual(p.value, 42)

    def test_float_value_sets_f_prefix(self):
        """Test that a float value sets the 'F' suffix prefix."""
        p = Parameter({'temp': 3.14})
        self.assertEqual(p.prefix, 'F')

    def test_non_float_value_has_empty_prefix(self):
        """Test that a non-float value has an empty prefix."""
        p = Parameter({'count': 100})
        self.assertEqual(p.prefix, '')

    def test_str_representation_plain(self):
        """Test __str__ for a plain name=value pair."""
        p = Parameter({'myField': 'uint32_t'})
        self.assertEqual(str(p), '.myField=uint32_t')

    def test_str_representation_with_float_suffix(self):
        """Test __str__ includes the F suffix for float values."""
        p = Parameter({'val': 1.5})
        self.assertEqual(str(p), '.val=1.5F')

    def test_nested_dict_value_becomes_parameter(self):
        """Test that a dict value is recursively converted to a Parameter."""
        p = Parameter({'nested': {'inner': 'float'}})
        self.assertIsInstance(p.value, Parameter)
        self.assertEqual(p.value.name, 'inner')

    def test_nested_dict_sets_struct_delimiters(self):
        """Test that a nested dict value causes struct_begin='{' and struct_ned='}'."""
        p = Parameter({'nested': {'inner': 'float'}})
        self.assertEqual(p.struct_begin, '{')
        self.assertEqual(p.struct_ned, '}')

    def test_non_nested_has_no_struct_delimiters(self):
        """Test that a flat value leaves struct_begin and struct_ned empty."""
        p = Parameter({'flat': 'uint8_t'})
        self.assertEqual(p.struct_begin, '')
        self.assertEqual(p.struct_ned, '')


class TestVersion(unittest.TestCase):
    """Tests for the Version helper class."""

    def test_correct_version_parsed(self):
        """Test that a valid version string is split into major, minor, build."""
        v = Version('1.2.3')
        self.assertEqual(v.major, '1')
        self.assertEqual(v.minor, '2')
        self.assertEqual(v.build, '3')

    def test_str_representation(self):
        """Test that __str__ reproduces the original version format."""
        v = Version('2.10.99')
        self.assertEqual(str(v), '2.10.99')

    def test_zero_version(self):
        """Test that '0.0.0' is parsed correctly."""
        v = Version('0.0.0')
        self.assertEqual(v.major, '0')
        self.assertEqual(v.minor, '0')
        self.assertEqual(v.build, '0')

    def test_invalid_version_too_few_parts_raises(self):
        """Test that a version with fewer than 3 parts raises ValueError."""
        self.assertRaises(ValueError, Version, '1.0')

    def test_invalid_version_too_many_parts_raises(self):
        """Test that a version with more than 3 parts raises ValueError."""
        self.assertRaises(ValueError, Version, '1.0.0.0')


class TestParameterList(unittest.TestCase):
    """Tests for the ParameterList helper class."""

    def test_flat_dict_creates_parameters(self):
        """Test that a flat dict produces one Parameter per key."""
        pl = ParameterList({'raw': 5555, 'value': 1.0})
        self.assertEqual(len(pl.params), 2)

    def test_str_wraps_in_braces(self):
        """Test that __str__ wraps content in curly braces."""
        pl = ParameterList({'x': 1})
        result = str(pl)
        self.assertTrue(result.startswith('{'))
        self.assertTrue(result.endswith('}'))

    def test_nested_dict_creates_nested_parameter_list(self):
        """Test that a nested dict value creates a nested ParameterList."""
        pl = ParameterList({'inner': {'a': 1}})
        self.assertIsInstance(pl.params[0], ParameterList)

    def test_params_are_sorted_by_key(self):
        """Test that parameters within a ParameterList are ordered by key."""
        pl = ParameterList({'z': 1, 'a': 2})
        names = [p.name if hasattr(p, 'name') else None for p in pl.params]
        self.assertEqual(names, sorted([n for n in names if n]))


class TestValidateDataDefaultStruct(unittest.TestCase):
    """Tests for validate_data_default_struct."""

    def test_none_default_is_noop(self):
        """Test that None default does not raise and returns without modification."""
        validate_data_default_struct(None)  # should not raise

    def test_none_string_default_is_noop(self):
        """Test that the string 'None' is accepted without modification."""
        validate_data_default_struct('None')  # should not raise

    def test_plain_string_is_noop(self):
        """Test that a non-dict string default is accepted without modification."""
        validate_data_default_struct('Active')  # should not raise

    def test_integer_default_is_noop(self):
        """Test that an integer default (non-dict) is accepted without modification."""
        validate_data_default_struct(42)  # should not raise

    def test_dict_default_gets_string_key(self):
        """Test that a dict default is augmented with a 'string' key."""
        default = {'value': 1.0, 'raw': 100}
        validate_data_default_struct(default)
        self.assertIn('string', default)

    def test_dict_default_string_contains_field_names(self):
        """Test that the generated 'string' value references all field names."""
        default = {'raw': 5555, 'value': 123.0}
        validate_data_default_struct(default)
        self.assertIn('raw', default['string'])
        self.assertIn('value', default['string'])

    def test_dict_default_string_has_struct_syntax(self):
        """Test that the generated 'string' is wrapped in curly braces (struct init)."""
        default = {'x': 1}
        validate_data_default_struct(default)
        self.assertTrue(default['string'].startswith('{'))
        self.assertTrue(default['string'].endswith('}'))


if __name__ == '__main__':
    unittest.main()
