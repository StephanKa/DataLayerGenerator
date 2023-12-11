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
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'READWRITE',
             'namespace': 'Testify', 'version': '1.0.1'}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_multiple_correct_data_points(self):
        """Test multiple correct data point definitions."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test2', 'group': 'DefaultGroup', 'id': 2, 'type': 'int32_t', 'access': 'READWRITE',
             'namespace': 'Testify', 'version': '1.0.1'},
            {'name': 'test3', 'group': 'DefaultGroup', 'id': 3, 'type': 'int32_t', 'access': 'READONLY',
             'namespace': '', 'version': '1.0.1'}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_base_types(self):
        """Test all base types."""
        for base_type in generator.BASE_TYPES:
            test_data = [
                {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': base_type, 'access': 'READWRITE',
                 'namespace': '', 'version': '1.0.1'}]
            generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_duplicate_names(self):
        """Test for getting exception with duplicate names."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test', 'group': 'DefaultGroup', 'id': 2, 'type': 'int32_t', 'access': 'WRITEONLY',
             'namespace': '', 'version': '1.0.1'}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, struct_list={},
                          enum_list={}, custom_type={})

    def test_same_id_but_different_group(self):
        """Test same id but in different groups."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test2', 'group': 'DefaultGroup2', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY',
             'namespace': '', 'version': '1.0.1'}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_same_name_but_different_group(self):
        """Test same name but different, which leads to errors."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test', 'group': 'DefaultGroup2', 'id': 2, 'type': 'int32_t', 'access': 'WRITEONLY',
             'namespace': '', 'version': '1.0.1'}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, struct_list={},
                          enum_list={}, custom_type={})

    def test_same_name_but_namespace(self):
        """Test same name but in different namespaces."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY',
             'namespace': '', 'version': '1.0.1'},
            {'name': 'test', 'group': 'DefaultGroup', 'id': 2, 'type': 'int32_t', 'access': 'WRITEONLY',
             'namespace': 'Test', 'version': '1.0.1'}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_correct_data_point_custom_struct(self):
        """Test correct data point with custom defined struct."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct', 'access': 'READWRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1'}]
        generator.data_point_validator(test_data, {'TestStruct'}, enum_list={}, custom_type={})

    def test_correct_data_point_failing_custom_struct(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct2', 'access': 'READWRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1'}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, {'TestStruct'},
                          enum_list={}, custom_type={})

    def test_correct_data_point_array_size_custom_struct(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct', 'access': 'READWRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'arraySize': 10}]
        generator.data_point_validator(test_data, {'TestStruct'}, enum_list={}, custom_type={})

    def test_incorrect_data_point_array_size_custom_struct(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct2', 'access': 'READWRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'arraySize': 0}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, {'TestStruct'},
                          enum_list={}, custom_type={})

    def test_correct_data_point_array_size(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'READWRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'arraySize': 10}]
        generator.data_point_validator(test_data, struct_list={}, enum_list={}, custom_type={})

    def test_incorrect_data_point_array_size(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'READWRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'arraySize': 0}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, struct_list={},
                          enum_list={}, custom_type={})

    def test_correct_enum_data_point(self):
        """Test correct data point with enum type with values only."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'Status', 'access': 'READWRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'default': 'Booting'}]
        generator.data_point_validator(test_data, struct_list={},
                                       enum_list=[{'name': 'Status', 'type': 'uint32_t', 'values': [
                                           'Booting', 'Starting', 'Finished'], 'autoId': True}], custom_type={})

    def test_correct_enum_data_point_key_value(self):
        """Test correct data point with enum type with key value pair."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'ErrorStates', 'access': 'READWRITE',
                      'namespace': 'Testify',
                      'version': '1.0.1', 'default': 'Warn'}]
        generator.data_point_validator(test_data, struct_list={},
                                       enum_list=[{'name': 'ErrorStates', 'type': 'uint32_t', 'values': [
                                           {'None': 0}, {'Info': 1}, {'Warn': 4}, {'Error': 5},
                                           {'Critical': 9}, {'Fatal': 10}], 'autoId': False}], custom_type={})


if __name__ == '__main__':
    unittest.main()
