"""Tests for the J-Link debug manifest generator."""
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class TestDebugManifest(unittest.TestCase):
    """Verify parsing the debug-export linker symbols."""

    def test_writes_debug_table_address_and_size(self):
        """The GNU LD map symbols become a machine-readable table manifest."""
        with tempfile.TemporaryDirectory() as temporary_directory:
            temporary_path = Path(temporary_directory)
            map_file = temporary_path / 'firmware.map'
            output_file = temporary_path / 'firmware.debug.json'
            map_file.write_text(
                '                0x08001234                __datalayer_debug_start = .\n'
                '                0x0800125c                __datalayer_debug_end = .\n',
                encoding='utf-8')

            subprocess.run([
                sys.executable, str(ROOT / 'scripts/create_debug_manifest.py'),
                '--map', str(map_file),
                '--output', str(output_file),
            ], check=True)

            self.assertEqual(json.loads(output_file.read_text(encoding='utf-8')),
                             {'tableAddress': 0x08001234, 'tableSize': 40})


if __name__ == '__main__':
    unittest.main()
