"""This modules defines some constants needed for parsing."""

BASE_TYPES = ['uint8_t', 'uint16_t', 'uint32_t', 'int8_t', 'int16_t', 'int32_t', 'float', 'double', 'bool', 'uint64_t', 'int64_t', 'std::string_view']
PREFIX_MAP = {'uint8_t': '', 'uint16_t': '', 'uint32_t': '', 'int8_t': '', 'int16_t': '', 'int32_t': '', 'float': 'F',
              'double': '', 'uint64_t': '', 'int64_t': '', 'bool': '', 'std::string_view': 'sv'}
SUPPORTED_PERSISTENCE = ['None', 'Cyclic', 'OnWrite']
BASE_ACCESS = ['READ_ONLY', 'WRITE_ONLY', 'READ_WRITE']
