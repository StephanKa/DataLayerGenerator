"""Test suite for PlantUML model rendering."""
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/')))
import generator  # noqa: F401 - ensures src/generator is appended to sys.path
from generator.umlGenerator import UML, generate_uml


class TestUmlGenerator(unittest.TestCase):
    """Tests for PlantUML rendering from validated generator models."""

    def setUp(self):
        """Create a model with enum, struct, alias type, and namespaced datapoints."""
        self.enums = generator.enum_validator([
            {'name': 'State', 'type': 'uint32_t', 'values': [{'Ready': 2}], 'autoId': False}
        ])
        self.types = generator.type_validator([
            {'name': 'Percent', 'type': 'uint8_t'}
        ])
        self.structs, struct_names = generator.struct_validator([
            {'name': 'Reading', 'parameter': [{'state': 'uint32_t'}, {'value': 'float'}]},
            {'name': 'Snapshot', 'parameter': [{'reading': 'Reading'}]},
        ])
        self.datapoints = generator.data_point_validator([
            {
                'name': 'level', 'group': 'Main Group', 'id': 1, 'type': 'Percent',
                'access': 'READ_WRITE', 'namespace': 'System Core', 'version': '1.2.3',
                'description': 'Current\nlevel', 'arraySize': 4,
            },
            {
                'name': 'state', 'group': 'Main Group', 'id': 2, 'type': 'State',
                'access': 'READ_ONLY', 'namespace': 'System Core', 'version': '1.2.3',
            },
            {
                'name': 'snapshot', 'group': 'Main Group', 'id': 3, 'type': 'Snapshot',
                'access': 'READ_WRITE', 'version': '1.2.3',
            },
        ], struct_names, self.enums, self.types)

    def test_renders_validated_enum_members_and_named_type_relationships(self):
        """Render transformed enum values and relationships to every named type."""
        output = str(UML(self.enums, self.structs, self.datapoints, self.types))

        self.assertIn('Ready = 2', output)
        self.assertIn('class "Percent" as type_Percent <<alias>>', output)
        self.assertIn('datapoint_Main_Group_System_Core_level *-- type_Percent', output)
        self.assertIn('datapoint_Main_Group_System_Core_state *-- enum_State', output)
        self.assertIn('datapoint_Main_Group__snapshot *-- struct_Snapshot', output)

    def test_consolidates_namespace_and_deduplicates_struct_relationships(self):
        """Render each namespace once and each struct composition edge once."""
        output = str(UML(self.enums, self.structs, self.datapoints, self.types))

        self.assertEqual(output.count('package "System Core"'), 1)
        self.assertEqual(output.count('struct_Snapshot *-- struct_Reading'), 1)
        self.assertIn('type: Percent[4]', output)
        self.assertIn('description: Current\\nlevel', output)

    def test_writes_utf8_plantuml_output(self):
        """Write the complete PlantUML document to the requested output directory."""
        with tempfile.TemporaryDirectory() as temporary_directory:
            generate_uml(self.enums, self.structs, self.datapoints, self.types, temporary_directory)
            output_path = os.path.join(temporary_directory, 'overview.plantuml')

            with open(output_path, encoding='utf-8') as output_file:
                output = output_file.read()

        self.assertTrue(output.startswith('@startuml\n'))
        self.assertTrue(output.endswith('\n@enduml'))


if __name__ == '__main__':
    unittest.main()
