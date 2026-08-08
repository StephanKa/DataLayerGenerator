"""Create a J-Link debug-export manifest from a GNU LD map file."""
import argparse
import json
import re
from pathlib import Path


def symbol_address(map_text, symbol):
    """Return a GNU LD map symbol address, or raise a clear error."""
    match = re.search(rf'^\s*(0x[0-9a-fA-F]+)\s+{re.escape(symbol)}(?:\s|=|$)', map_text, re.MULTILINE)
    if match is None:
        raise ValueError(f"Could not find '{symbol}' in GNU LD map output")
    return int(match.group(1), 16)


def main():
    """Parse arguments and write the debug-export table location as JSON."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', dest='map_file', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()

    map_text = args.map_file.read_text(encoding='utf-8')
    start = symbol_address(map_text, '__datalayer_debug_start')
    end = symbol_address(map_text, '__datalayer_debug_end')
    if end < start:
        raise ValueError('The debug export section has an invalid address range')

    args.output.write_text(json.dumps({'tableAddress': start, 'tableSize': end - start}, indent=2) + '\n',
                           encoding='utf-8')


if __name__ == '__main__':
    main()
