"""This modules defines some constants needed for parsing."""

BASE_TYPES = ['uint8_t', 'uint16_t', 'uint32_t', 'int8_t', 'int16_t', 'int32_t', 'float', 'double']
PREFIX_MAP = {'uint8_t': '', 'uint16_t': '', 'uint32_t': '', 'int8_t': '', 'int16_t': '', 'int32_t': '', 'float': 'F',
              'double': ''}
SUPPORTED_PERSISTENCE = ['None', 'Cyclic', 'OnWrite']
BASE_ACCESS = ['READONLY', 'WRITEONLY', 'READWRITE']
