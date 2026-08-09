"""Tests for the generated SEGGER RTT reader."""
import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class TestRttGenerator(unittest.TestCase):
    """Verify the RTT template is generated and decodes model values."""

    def test_generator_emits_metadata_and_model_contract(self):
        """The generator emits host metadata and Catch2 contract checks for every model."""
        with tempfile.TemporaryDirectory() as temporary_directory:
            output_directory = Path(temporary_directory)
            subprocess.run([
                sys.executable, str(ROOT / 'src/generator/generator.py'),
                '--model_dir', str(ROOT / 'src/model'),
                '--out_dir', str(output_directory),
                '--template_dir', str(ROOT / 'src/template'),
                '--schema_dir', str(ROOT / 'src/generator'),
            ], check=True)

            with (output_directory / 'generated/doc/model-metadata.json').open(encoding='utf-8') as metadata_file:
                metadata = json.load(metadata_file)
            contract = (output_directory / 'generated/test/modelTests.h').read_text(encoding='utf-8')
            datalayer_header = (output_directory / 'generated/include/datalayer.h').read_text(encoding='utf-8')

            self.assertEqual(metadata['formatVersion'], 1)
            self.assertGreater(metadata['summary']['datapoints'], 0)
            self.assertIn('Generated model identifier contract', contract)
            self.assertIn('Generated model access and serialization contract', contract)
            self.assertIn('Generated model range contract', contract)
            self.assertIn('namespace DataLayer::Model', datalayer_header)
            self.assertIn('findDatapoint', datalayer_header)

    def test_generated_reader_decodes_split_scalar_frame(self):
        """The reader waits for a complete frame and decodes its scalar payload."""
        with tempfile.TemporaryDirectory() as temporary_directory:
            output_directory = Path(temporary_directory)
            subprocess.run([
                sys.executable, str(ROOT / 'src/generator/generator.py'),
                '--model_dir', str(ROOT / 'src/model'),
                '--out_dir', str(output_directory),
                '--template_dir', str(ROOT / 'src/template'),
                '--schema_dir', str(ROOT / 'src/generator'),
            ], check=True)
            module_path = output_directory / 'generated/datalayer_example/rtt.py'
            specification = importlib.util.spec_from_file_location('generated_rtt', module_path)
            module = importlib.util.module_from_spec(specification)
            sys.modules[specification.name] = module
            specification.loader.exec_module(module)

            frame = module.FRAME_HEADER.pack(module.FRAME_MAGIC, module.PROTOCOL_VERSION, 0, 0x4001, 4)
            chunks = iter((frame[:3], frame[3:] + (4211).to_bytes(4, 'little', signed=True)))
            reader = module.RttReader(lambda: next(chunks, b''))

            self.assertEqual(reader.read(), [])
            self.assertEqual(reader.read(), [('Testify::test', 4211)])

    def test_generated_debug_reader_decodes_memory_export(self):
        """The debug reader decodes a readable datapoint through its export entry."""
        with tempfile.TemporaryDirectory() as temporary_directory:
            output_directory = Path(temporary_directory)
            subprocess.run([
                sys.executable, str(ROOT / 'src/generator/generator.py'),
                '--model_dir', str(ROOT / 'src/model'),
                '--out_dir', str(output_directory),
                '--template_dir', str(ROOT / 'src/template'),
                '--schema_dir', str(ROOT / 'src/generator'),
            ], check=True)
            module_path = output_directory / 'generated/datalayer_example/rtt.py'
            specification = importlib.util.spec_from_file_location('generated_debug', module_path)
            module = importlib.util.module_from_spec(specification)
            sys.modules[specification.name] = module
            specification.loader.exec_module(module)

            table_address = 0x08001000
            value_address = 0x20000100
            table = module.DEBUG_ENTRY.pack(0x4001, 4, value_address)
            memory = {table_address: table, value_address: (4211).to_bytes(4, 'little', signed=True)}
            reader = module.DebugMemoryReader(lambda address, size: memory[address][:size], table_address)

            self.assertEqual(reader.read(), [('Testify::test', 4211)])


if __name__ == '__main__':
    unittest.main()
