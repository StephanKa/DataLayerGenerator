"""Test suite for generator utility functions (create_group_data_point_dict)."""
import sys
import os
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/')))
import generator  # noqa: F401 — ensures src/generator/ is appended to sys.path
import generator.generator as _gen_module  # loads generator.py submodule explicitly
create_group_data_point_dict = _gen_module.create_group_data_point_dict
create_perfect_hash = _gen_module.create_perfect_hash
perfect_hash = _gen_module.perfect_hash


class TestCreateGroupDataPointDict(unittest.TestCase):
    """Tests for the create_group_data_point_dict utility function."""

    def test_single_datapoint_no_namespace(self):
        """Test single datapoint without namespace uses plain name."""
        dps = [{'name': 'temp', 'group': 'G1', 'namespace': ''}]
        result = create_group_data_point_dict(dps)
        self.assertEqual(result, {'G1': ['temp']})

    def test_single_datapoint_with_namespace(self):
        """Test single datapoint with namespace prepends 'namespace::' prefix."""
        dps = [{'name': 'temp', 'group': 'G1', 'namespace': 'NS'}]
        result = create_group_data_point_dict(dps)
        self.assertEqual(result, {'G1': ['NS::temp']})

    def test_multiple_datapoints_same_group(self):
        """Test multiple datapoints in the same group are all collected under one key."""
        dps = [
            {'name': 'a', 'group': 'G1', 'namespace': ''},
            {'name': 'b', 'group': 'G1', 'namespace': ''}
        ]
        result = create_group_data_point_dict(dps)
        self.assertIn('G1', result)
        self.assertIn('a', result['G1'])
        self.assertIn('b', result['G1'])
        self.assertEqual(len(result['G1']), 2)

    def test_multiple_groups(self):
        """Test datapoints across different groups produce separate keys."""
        dps = [
            {'name': 'a', 'group': 'G1', 'namespace': ''},
            {'name': 'b', 'group': 'G2', 'namespace': ''}
        ]
        result = create_group_data_point_dict(dps)
        self.assertIn('G1', result)
        self.assertIn('G2', result)
        self.assertEqual(result['G1'], ['a'])
        self.assertEqual(result['G2'], ['b'])

    def test_empty_datapoint_list(self):
        """Test that an empty list returns an empty dict."""
        result = create_group_data_point_dict([])
        self.assertEqual(result, {})

    def test_missing_namespace_key_uses_plain_name(self):
        """Test that a datapoint without a 'namespace' key uses the plain name."""
        dps = [{'name': 'temp', 'group': 'G1'}]
        result = create_group_data_point_dict(dps)
        self.assertEqual(result, {'G1': ['temp']})

    def test_empty_namespace_does_not_add_prefix(self):
        """Test that an empty namespace string does not add '::' to the name."""
        dps = [{'name': 'dp', 'group': 'G', 'namespace': ''}]
        result = create_group_data_point_dict(dps)
        self.assertNotIn('::', result['G'][0])

    def test_mixed_namespaced_and_plain_in_same_group(self):
        """Test a mix of namespaced and plain datapoints in the same group."""
        dps = [
            {'name': 'dp1', 'group': 'G', 'namespace': 'NS'},
            {'name': 'dp2', 'group': 'G', 'namespace': ''}
        ]
        result = create_group_data_point_dict(dps)
        self.assertIn('NS::dp1', result['G'])
        self.assertIn('dp2', result['G'])

    def test_multiple_namespaces_same_group(self):
        """Test multiple different namespaces within the same group are all preserved."""
        dps = [
            {'name': 'dp1', 'group': 'G', 'namespace': 'Alpha'},
            {'name': 'dp2', 'group': 'G', 'namespace': 'Beta'}
        ]
        result = create_group_data_point_dict(dps)
        self.assertIn('Alpha::dp1', result['G'])
        self.assertIn('Beta::dp2', result['G'])

    def test_insertion_order_preserved(self):
        """Test that datapoints appear in insertion order within their group."""
        dps = [
            {'name': 'first', 'group': 'G', 'namespace': ''},
            {'name': 'second', 'group': 'G', 'namespace': ''},
            {'name': 'third', 'group': 'G', 'namespace': ''}
        ]
        result = create_group_data_point_dict(dps)
        self.assertEqual(result['G'], ['first', 'second', 'third'])


class TestCreatePerfectHash(unittest.TestCase):
    """Tests for the generated two-level datapoint ID lookup table."""

    def test_current_and_legacy_ids_resolve_to_their_datapoint(self):
        """Test current and legacy IDs map to their owning datapoint index."""
        data_points = [
            {'group': 'G1', 'id': 1, 'renamedFrom': [7]},
            {'group': 'G2', 'id': 2, 'renamedFrom': []},
        ]
        groups = [
            {'name': 'G1', 'baseId': '0x1000'},
            {'name': 'G2', 'baseId': '0x2000'},
        ]

        result = create_perfect_hash(data_points, groups)

        self.assertEqual(result['bucket_count'], 3)
        resolved = {}
        for bucket in result['buckets']:
            for slot_offset in range(bucket['slot_count']):
                slot = result['slots'][bucket['offset'] + slot_offset]
                if slot['occupied']:
                    resolved[slot['id']] = slot['index']
        self.assertEqual(resolved, {0x1001: 0, 0x1007: 0, 0x2002: 1})

    def test_every_entry_is_at_its_hashed_slot(self):
        """Test each populated slot is located by its primary and secondary hash."""
        data_points = [
            {'group': 'G', 'id': identifier, 'renamedFrom': []}
            for identifier in [1, 2, 3, 4, 5, 100, 1000]
        ]
        result = create_perfect_hash(data_points, [{'name': 'G', 'baseId': '0x4000'}])

        for bucket_index, bucket in enumerate(result['buckets']):
            for slot_offset in range(bucket['slot_count']):
                slot = result['slots'][bucket['offset'] + slot_offset]
                if slot['occupied']:
                    self.assertEqual(perfect_hash(slot['id'], 0) % result['bucket_count'], bucket_index)
                    self.assertEqual(perfect_hash(slot['id'], bucket['seed']) % bucket['slot_count'], slot_offset)

    def test_duplicate_absolute_id_raises(self):
        """Test colliding absolute IDs are rejected during table generation."""
        data_points = [
            {'group': 'G1', 'id': 2, 'renamedFrom': []},
            {'group': 'G2', 'id': 1, 'renamedFrom': []},
        ]
        groups = [
            {'name': 'G1', 'baseId': '0x1000'},
            {'name': 'G2', 'baseId': '0x1001'},
        ]

        with self.assertRaises(_gen_module.DatapointException):
            create_perfect_hash(data_points, groups)


if __name__ == '__main__':
    unittest.main()
