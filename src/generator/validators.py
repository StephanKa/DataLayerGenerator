"""This module whols all defined validators for enum, group, struct and data points."""
from constants import BASE_ACCESS, BASE_TYPES, SUPPORTED_PERSISTENCE, PREFIX_MAP
from validatorException import EnumException, StructException, GroupException, DatapointException, TypeException


class ParameterList:
    """Class for collecting and nesting parameter."""

    def __init__(self, default_dict):
        """Collect list of parameter.

        :param default_dict: dictionary with default parameter definitions
        """
        self.params = list()
        for param in sorted(default_dict):
            if isinstance(default_dict[param], dict):
                self.params.append(ParameterList(default_dict[param]))
                continue
            self.params.append(Parameter({param: default_dict[param]}))

    def __str__(self):
        """Concatenate parameter in recursive manner.

        :return concatenated string for all parameter in list
        """
        return_str = '{'
        return_str += ', '.join([str(i) for i in self.params])
        return_str += '}'
        return return_str


class Parameter:
    """Intermediate class for parsing json data to struct for jinja."""

    def __init__(self, value):
        """Split to name and value.

        :param value: dictionary for splitting
        """
        key, val = zip(*value.items())
        self.name = key[0]
        self.value = val[0]
        self.prefix = ''

        self.struct_begin = ''
        self.struct_ned = ''
        if isinstance(self.value, dict):
            self.value = Parameter(self.value)
            self.struct_begin = '{'
            self.struct_ned = '}'
        elif isinstance(self.value, float):
            self.prefix = PREFIX_MAP['float']

    def __str__(self):
        """Create string for structs and value assignment, including prefix.

        :return concatenated current parameter
        """
        return f'.{self.name}={self.struct_begin}{self.value}{self.prefix}{self.struct_ned}'


class Version:
    """Intermedoate class for parsing json data to struct for jinja."""

    def __init__(self, version_str):
        """Split to major, minor and build version information.

        :param version_str: given the version with default formatting <MAJOR>.<MINOR>.<BUILD>
        """
        self.major, self.minor, self.build = version_str.split('.')

    def __str__(self):
        """Create string for version information.

        :return concatenated current version
        """
        return f'{self.major}.{self.minor}.{self.build}'


def enum_validator(enum_data):
    """Check the given enum data for consistency, duplicated and autoId.

    :param enum_data: dictionary of all enum definitions
    :return: given enum_data
    """
    check_names = list()
    for temp_enum in enum_data:
        name = temp_enum['name'].strip()
        if name.lower() in check_names:
            raise EnumException(f"Enum name '{name}' already defined, please check your model")
        check_names.append(name.lower())
        if temp_enum['type'] not in BASE_TYPES:
            raise EnumException(f"Enum type '{temp_enum['type']}' is not supported")
        if 'autoId' in temp_enum:
            if temp_enum['autoId'] and len(temp_enum['values']) > 0 and not isinstance(temp_enum['values'][0], str):
                raise EnumException('Enum autoId is true but values are not string only')
            elif not temp_enum['autoId'] and len(temp_enum['values']) > 0 and isinstance(temp_enum['values'][0], str):
                raise EnumException('Enum autoId is false defined but values are string only')
        if 'autoId' not in temp_enum or not temp_enum['autoId']:
            for index, temp_values in enumerate(temp_enum['values']):
                if isinstance(temp_values, str):
                    raise EnumException('Enum autoId is missing and only value was given')
                temp = Parameter(temp_values)
                temp_enum['values'][index] = temp
    return enum_data


def group_validator(group_data):
    """
    Check the given group data for consistency, duplicated and autoId.

    :param group_data: dictionary of all group definitions
    :return: given group_data
    """
    check_names = dict()
    base_id = list()
    for temp_group in group_data:
        if temp_group['name'] in check_names:
            raise GroupException(f"Group name '{temp_group['name']}' already defined, please check your model")
        check_names[temp_group['name']] = None
        if 'description' not in temp_group:
            temp_group['description'] = None
        if temp_group['persistence'] not in SUPPORTED_PERSISTENCE:
            raise GroupException(f"Persistence type '{temp_group['persistence']}' is not supported")
        if temp_group['baseId'] in base_id:
            raise GroupException(f"Group baseId '{temp_group['baseId']}' already defined, please check your model")
        if 'version' in temp_group:
            temp_group['version'] = Version(temp_group['version'])
        if 'allowUpgrade' not in temp_group:
            temp_group['allowUpgrade'] = 'false'
        elif temp_group['allowUpgrade']:
            temp_group['allowUpgrade'] = 'true'
        else:
            temp_group['allowUpgrade'] = 'false'
        base_id.append(temp_group['baseId'])
    return group_data


def struct_validator(struct_data):
    """
    Check the given struct data for consistency, duplicated and autoId.

    :param struct_data: dictionary of all struct definitions
    :return: given struct_data
    """
    check_names = dict()
    struct_type_names = [i['name'] for i in struct_data]
    for temp_group in struct_data:
        if temp_group['name'] in check_names:
            raise StructException(f"Struct name '{temp_group['name']}' already defined, please check your model")
        check_names[temp_group['name']] = None
        parameter_names = list()
        if len(temp_group['parameter']) == 0:
            raise StructException(f"Struct '{temp_group['name']}' doesn't have any parameter defined")
        temp_group['parameter'] = sorted(temp_group['parameter'], key=lambda d: list(d.keys()))
        for index, temp_parameter in enumerate(temp_group['parameter']):
            temp = Parameter(temp_parameter)
            if temp.name in parameter_names:
                raise StructException(f"Struct name '{temp_group['name']}' already defined, please check your model")
            parameter_names.append(temp.name)
            if temp.value not in BASE_TYPES and temp.value not in struct_type_names:
                raise StructException(f"Parameter type '{temp.value}' is not supported")
            temp_group['parameter'][index] = temp
    return struct_data, check_names


def validate_data_default_struct(default=None):
    """
    Check nested structures in defined datapoint.

    :param default: defines information about default initialization
    """
    if default is None or default == 'None' or not isinstance(default, dict):
        return
    default['string'] = str(ParameterList(default))


def data_point_validator(data_point_data, struct_list, enum_list, custom_type):
    """
    Check the given data point for consistency and if given struct is defined.

    :param data_point_data: dictionary of all data points definitions
    :param struct_list: dictionary of all struct definitions
    :param enum_list: dictionary of available custom-made enumerations
    :param custom_type: dictionary of available custom-made types
    :return: given data_point_data
    """
    check_names = dict()
    group_id = dict()
    for temp_dp in data_point_data:
        group = temp_dp['group']
        name = temp_dp['name']
        access = temp_dp['access']
        dp_id = temp_dp['id']
        dp_type = temp_dp['type']
        if 'namespace' in temp_dp:
            name = '{}::{}'.format(temp_dp['namespace'], name)
        else:
            temp_dp['namespace'] = ''
        if 'description' not in temp_dp:
            temp_dp['description'] = None
        if name in check_names:
            raise DatapointException(f"Datapoint name '{name}' already defined, please check your model")
        check_names[name] = None
        if access not in BASE_ACCESS:
            raise DatapointException(f"Datapoint access type '{access}' is not supported")
        if (dp_type not in BASE_TYPES and dp_type not in struct_list and dp_type not in [i['name'] for i in enum_list]
                and dp_type not in [i['name'] for i in custom_type]):
            raise DatapointException(f"Datapoint type '{dp_type}' is not supported")
        if 'default' in temp_dp:
            validate_data_default_struct(temp_dp['default'])
        if group not in group_id:
            group_id[group] = list()
        if group in group_id and dp_id in group_id[group]:
            raise DatapointException(f"Datapoint id '{dp_id}' is already registered for group '{group}'")
        if 'version' in temp_dp:
            temp_dp['version'] = Version(temp_dp['version'])
        if 'arraySize' in temp_dp:
            dp_array_size = temp_dp['arraySize']
            if dp_array_size == 0:
                raise DatapointException(f"Datapoint has arraySize defined but size is '{dp_array_size}'")
        else:
            temp_dp['arraySize'] = 0
        if 'allowUpgrade' not in temp_dp:
            temp_dp['allowUpgrade'] = 'false'
        elif temp_dp['allowUpgrade']:
            temp_dp['allowUpgrade'] = 'true'
        else:
            temp_dp['allowUpgrade'] = 'false'
        group_id[group].append(dp_id)
    return data_point_data


def type_validator(type_data):
    """
    Check the given type data for consistency and duplication.

    :param type_data: dictionary of all type definitions
    :return: given type_data
    """
    check_names = dict()
    for temp_group in type_data:
        if temp_group['name'] in check_names:
            raise TypeException(f"Type name '{temp_group['name']}' already defined, please check your model")
        check_names[temp_group['name']] = None
        if 'type' not in temp_group:
            raise TypeException(f"Typed definition is missing 'type', please check your model")
        if temp_group['type'] not in BASE_TYPES:
            raise TypeException(f"Type '{temp_group['type']}' is not defined in base types")
        if 'min' not in temp_group:
            temp_group['min'] = None
        if 'max' not in temp_group:
            temp_group['max'] = None
        if 'min' in temp_group and 'max' in temp_group and temp_group['min'] is not None and temp_group['max'] is not None:
            if temp_group['min'] == temp_group['max']:
                raise TypeException(f"Min and Max are equal! max({temp_group['max']}) and min({temp_group['min']})")
            elif temp_group['min'] > temp_group['max']:
                raise TypeException(f"For min and max wrong: max({temp_group['max']}) and min({temp_group['min']})")
    return type_data
