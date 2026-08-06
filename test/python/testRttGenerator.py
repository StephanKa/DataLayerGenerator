"""Tests for the generated SEGGER RTT reader."""
import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class TestRttGenerator(unittest.TestCase):
    """Verify the RTT template is generated and decodes model values."""

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


if __name__ == '__main__':
    unittest.main()
