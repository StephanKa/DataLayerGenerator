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
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'READWRITE', 'namespace': 'Testify', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        generator.data_point_validator(test_data, {})

    def test_multiple_correct_data_points(self):
        """Test multiple correct data point definitions."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}},
            {'name': 'test2', 'group': 'DefaultGroup', 'id': 2, 'type': 'int32_t', 'access': 'READWRITE', 'namespace': 'Testify', 'version': {'major': 1, 'minor': 0, 'build': 1}},
            {'name': 'test3', 'group': 'DefaultGroup', 'id': 3, 'type': 'int32_t', 'access': 'READONLY', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        generator.data_point_validator(test_data, {})

    def test_base_types(self):
        """Test all base types."""
        for base_type in generator.BASE_TYPES:
            test_data = [
                {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': base_type, 'access': 'READWRITE', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
            generator.data_point_validator(test_data, {})

    def test_duplicate_names(self):
        """Test for getting exception with duplicate names."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}},
            {'name': 'test', 'group': 'DefaultGroup', 'id': 2, 'type': 'int32_t', 'access': 'WRITEONLY', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, {})

    def test_same_id_but_different_group(self):
        """Test same id but in different groups."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}},
            {'name': 'test2', 'group': 'DefaultGroup2', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        generator.data_point_validator(test_data, {})

    def test_same_name_but_different_group(self):
        """Test same name but different, which leads to errors."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}},
            {'name': 'test', 'group': 'DefaultGroup2', 'id': 2, 'type': 'int32_t', 'access': 'WRITEONLY', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, {})

    def test_same_name_but_namespace(self):
        """Test same name but in different namespaces."""
        test_data = [
            {'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'int32_t', 'access': 'WRITEONLY', 'namespace': '', 'version': {'major': 1, 'minor': 0, 'build': 1}},
            {'name': 'test', 'group': 'DefaultGroup', 'id': 2, 'type': 'int32_t', 'access': 'WRITEONLY', 'namespace': 'Test', 'version': {'major': 1, 'minor': 0, 'build': 1}}]
        generator.data_point_validator(test_data, {})

    def test_correct_data_point_custom_struct(self):
        """Test correct data point with custom defined struct."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct', 'access': 'READWRITE', 'namespace': 'Testify',
                      'version': {'major': 1, 'minor': 0, 'build': 1}}]
        generator.data_point_validator(test_data, {'TestStruct'})

    def test_correct_data_point_failing_custom_struct(self):
        """Test correct data point with not existing struct type."""
        test_data = [{'name': 'test', 'group': 'DefaultGroup', 'id': 1, 'type': 'TestStruct2', 'access': 'READWRITE', 'namespace': 'Testify',
                      'version': {'major': 1, 'minor': 0, 'build': 1}}]
        self.assertRaises(generator.DatapointException, generator.data_point_validator, test_data, {'TestStruct'})


if __name__ == '__main__':
    unittest.main()
