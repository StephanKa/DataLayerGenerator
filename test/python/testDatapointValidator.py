"""Test suite to test the data point validator."""
import sys
import os
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/')))
import generator


class TestDatapointValidator(unittest.TestCase):
    """Test suite to test the data point validator."""

    def test_correct_data_point(self):
        """Test correct data point definition."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'READ_WRITE',
             'namespace': 'Testify', 'version': '1.0.1'}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_multiple_correct_data_points(self):
        """Test multiple correct data point definitions."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITE_ONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test2', 'group': 'DefaultGroup', 'id': 2, 'type': 'int32_t', 'access': 'READ_WRITE',
             'namespace': 'Testify', 'version': '1.0.1'},
            {'name': 'test3', 'group': 'DefaultGroup', 'id': 3, 'type': 'int32_t', 'access': 'READ_ONLY',
             'namespace': '', 'version': '1.0.1'}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_base_types(self):
        """Test all base types."""
        for base_type in generator.BASE_TYPES:
            test_data = [
                {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': base_type, 'access': 'READ_WRITE',
                 'namespace': '', 'version': '1.0.1'}]
            generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_duplicate_names(self):
        """Test for getting exception with duplicate names."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITE_ONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test', 'group': 'DefaultGroup', 'id': 2, 'type': 'int32_t', 'access': 'WRITE_ONLY',
             'namespace': '', 'version': '1.0.1'}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, struct_list={},
                          enum_list={}, custom_type={})

    def test_same_id_but_different_group(self):
        """Test same id but in different groups."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITE_ONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test2', 'group': 'DefaultGroup2', 'id': 1, 'type': 'int32_t', 'access': 'WRITE_ONLY',
             'namespace': '', 'version': '1.0.1'}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_same_name_but_different_group(self):
        """Test same name but different, which leads to errors."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITE_ONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test', 'group': 'DefaultGroup2', 'id': 2, 'type': 'int32_t', 'access': 'WRITE_ONLY',
             'namespace': '', 'version': '1.0.1'}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, struct_list={},
                          enum_list={}, custom_type={})

    def test_same_name_but_namespace(self):
        """Test same name but in different namespaces."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITE_ONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test', 'group': 'DefaultGroup', 'id': 2, 'type': 'int32_t', 'access': 'WRITE_ONLY',
             'namespace': 'Test', 'version': '1.0.1'}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_correct_data_point_custom_struct(self):
        """Test correct data point with custom defined struct."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct', 'access': 'READ_WRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1'}]
        generator.data_point_validator(test_data, {'TestStruct'}, enum_list={}, custom_type={})

    def test_correct_data_point_failing_custom_struct(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct2', 'access': 'READ_WRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1'}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, {'TestStruct'},
                          enum_list={}, custom_type={})

    def test_correct_data_point_array_size_custom_struct(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct', 'access': 'READ_WRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'arraySize': 10}]
        generator.data_point_validator(test_data, {'TestStruct'}, enum_list={}, custom_type={})

    def test_incorrect_data_point_array_size_custom_struct(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct2', 'access': 'READ_WRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'arraySize': 0}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, {'TestStruct'},
                          enum_list={}, custom_type={})

    def test_correct_data_point_array_size(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'READ_WRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'arraySize': 10}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_incorrect_data_point_array_size(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'READ_WRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'arraySize': 0}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, struct_list={},
                          enum_list={}, custom_type={})

    def test_correct_enum_data_point(self):
        """Test correct data point with enum type with values only."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'Status', 'access': 'READ_WRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'default': 'Booting'}]
        generator.data_point_validator(test_data, struct_list={},
                                       enum_list=[{'name': 'Status', 'type': 'uint32_t', 'values': [
                                           'Booting', 'Starting', 'Finished'], 'autoId': True}], custom_type={})

    def test_correct_enum_data_point_key_value(self):
        """Test correct data point with enum type with key value pair."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'ErrorStates', 'access': 'READ_WRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'default': 'Warn'}]
        generator.data_point_validator(test_data, struct_list={},
                                       enum_list=[{'name': 'ErrorStates', 'type': 'uint32_t', 'values': [
                                           {'None': 0}, {'Info': 1}, {'Warn': 4}, {'Error': 5},
                                           {'Critical': 9}, {'Fatal': 10}], 'autoId': False}], custom_type={})

    def test_duplicate_id_same_group_raises(self):
        """Test that duplicate data point IDs within the same group raise DatapointException."""
        test_data = [
            {'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'int32_t', 'access': 'READ_WRITE',
             'namespace': '', 'version': '1.0.0'},
            {'name': 'dp2', 'group': 'G', 'id': 1, 'type': 'int32_t', 'access': 'READ_WRITE',
             'namespace': '', 'version': '1.0.0'}
        ]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data,
                          struct_list={}, enum_list={}, custom_type={})

    def test_missing_namespace_defaults_empty_string(self):
        """Test that a missing namespace field defaults to empty string."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'int32_t',
                      'access': 'READ_WRITE', 'version': '1.0.0'}]
        result = generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})
        self.assertEqual(result[0]['namespace'], '')

    def test_missing_description_defaults_none(self):
        """Test that a missing description field defaults to None."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'int32_t',
                      'access': 'READ_WRITE', 'version': '1.0.0'}]
        result = generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})
        self.assertIsNone(result[0]['description'])

    def test_missing_arraySize_defaults_zero(self):
        """Test that a missing arraySize field defaults to 0."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'int32_t',
                      'access': 'READ_WRITE', 'version': '1.0.0'}]
        result = generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})
        self.assertEqual(result[0]['arraySize'], 0)

    def test_allow_upgrade_true_becomes_string(self):
        """Test that allowUpgrade=True is converted to the string 'true'."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'int32_t',
                      'access': 'READ_WRITE', 'version': '1.0.0', 'allowUpgrade': True}]
        result = generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})
        self.assertEqual(result[0]['allowUpgrade'], 'true')

    def test_missing_allow_upgrade_defaults_false(self):
        """Test that a missing allowUpgrade field defaults to string 'false'."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'int32_t',
                      'access': 'READ_WRITE', 'version': '1.0.0'}]
        result = generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})
        self.assertEqual(result[0]['allowUpgrade'], 'false')

    def test_invalid_access_type_raises(self):
        """Test that an unsupported access type raises DatapointException."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'int32_t',
                      'access': 'ADMIN', 'version': '1.0.0'}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data,
                          struct_list={}, enum_list={}, custom_type={})

    def test_invalid_type_raises(self):
        """Test that an unrecognized datapoint type raises DatapointException."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'matrix_t',
                      'access': 'READ_WRITE', 'version': '1.0.0'}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data,
                          struct_list={}, enum_list={}, custom_type={})

    def test_all_access_types_valid(self):
        """Test that each supported access type is accepted."""
        for access in generator.BASE_ACCESS:
            test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'int32_t',
                          'access': access, 'version': '1.0.0'}]
            generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_custom_type_valid(self):
        """Test that a datapoint using a registered custom type is accepted."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'MyAlias',
                      'access': 'READ_WRITE', 'version': '1.0.0'}]
        generator.data_point_validator(test_data, struct_list={}, enum_list=[],
                                       custom_type=[{'name': 'MyAlias', 'type': 'uint32_t'}])

    def test_struct_default_dict_gets_string_key(self):
        """Test that a dict default value is augmented with a 'string' initialization key."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'TempStruct',
                      'access': 'READ_WRITE', 'version': '1.0.0',
                      'default': {'value': 1.0, 'raw': 100}}]
        result = generator.data_point_validator(test_data, struct_list={'TempStruct'},
                                                enum_list={}, custom_type={})
        self.assertIn('string', result[0]['default'])

    def test_empty_datapoint_list(self):
        """Test that an empty list is valid and returns an empty list."""
        result = generator.data_point_validator([], struct_list={}, enum_list={}, custom_type={})
        self.assertEqual(result, [])

    def test_version_parsed_to_object(self):
        """Test that the version string is replaced by a Version-like object."""
        test_data = [{'name': 'dp1', 'group': 'G', 'id': 1, 'type': 'int32_t',
                      'access': 'READ_WRITE', 'version': '2.1.3'}]
        result = generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})
        self.assertEqual(str(result[0]['version']), '2.1.3')


if __name__ == '__main__':
    unittest.main()
