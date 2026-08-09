"""Tests for IPO processor model validation."""
import os
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../../src/')))
from generator.validators import ProcessorException, processor_validator


class TestProcessorValidator(unittest.TestCase):
    """Exercise processor references, access directions, and dependency cycles."""

    @staticmethod
    def datapoints():
        """Return datapoints with valid input and output access modes."""
        return [
            {'name': 'input', 'namespace': '', 'access': 'READ_WRITE'},
            {'name': 'output', 'namespace': '', 'access': 'READ_WRITE'},
            {'name': 'readOnly', 'namespace': '', 'access': 'READ_ONLY'},
            {'name': 'writeOnly', 'namespace': '', 'access': 'WRITE_ONLY'},
        ]

    def test_valid_on_change_processor(self):
        """A valid processor retains its requested execution mode."""
        processors = [{'name': 'Control', 'function': 'runControl', 'inputs': ['input'],
                       'outputs': ['output'], 'execution': 'on_change'}]
        self.assertEqual(processor_validator(processors, self.datapoints())[0]['execution'], 'on_change')

    def test_missing_execution_defaults_to_manual(self):
        """Omitted execution mode is explicit manual scheduling."""
        processors = [{'name': 'Control', 'function': 'runControl', 'inputs': ['input'], 'outputs': ['output']}]
        self.assertEqual(processor_validator(processors, self.datapoints())[0]['execution'], 'manual')

    def test_write_only_input_is_rejected(self):
        """Processors cannot read WRITE_ONLY datapoints."""
        processors = [{'name': 'Control', 'function': 'runControl', 'inputs': ['writeOnly'], 'outputs': ['output']}]
        with self.assertRaisesRegex(ProcessorException, 'WRITE_ONLY'):
            processor_validator(processors, self.datapoints())

    def test_read_only_output_is_rejected(self):
        """Processors cannot declare READ_ONLY datapoints as outputs."""
        processors = [{'name': 'Control', 'function': 'runControl', 'inputs': ['input'], 'outputs': ['readOnly']}]
        with self.assertRaisesRegex(ProcessorException, 'READ_ONLY'):
            processor_validator(processors, self.datapoints())

    def test_dependency_cycle_is_rejected(self):
        """Output-to-input cycles are rejected before generation."""
        processors = [
            {'name': 'First', 'function': 'first', 'inputs': ['input'], 'outputs': ['output']},
            {'name': 'Second', 'function': 'second', 'inputs': ['output'], 'outputs': ['input']},
        ]
        with self.assertRaisesRegex(ProcessorException, 'cycle'):
            processor_validator(processors, self.datapoints())


if __name__ == '__main__':
    unittest.main()
