"""This module whols all defined validators for enum, group, struct and data points."""
from constants import BASE_ACCESS, BASE_TYPES, SUPPORTED_PERSISTENCE
from validatorException import EnumException, StructException, GroupException, DatapointException


def enum_validator(enum_data):
    """Check the given enum data for consistency, duplicated and autoId.

    :param enum_data: dictionary of all enum definitions
    :return: given enum_data
    """
    check_names = dict()
    for temp_enum in enum_data:
        if temp_enum['name'] in check_names:
            raise EnumException("Enum name '{}' already defined, please check your model".format(temp_enum['name']))
        check_names[temp_enum['name']] = None
        if temp_enum['type'] not in BASE_TYPES:
            raise EnumException("Enum type '{}' is not supported".format(temp_enum['type']))
        if 'autoId' in temp_enum:
            if temp_enum['autoId'] and len(temp_enum['values']) > 0 and not isinstance(temp_enum['values'][0], str):
                raise EnumException('Enum autoId is true but values are not string only')
            elif not temp_enum['autoId'] and len(temp_enum['values']) > 0 and isinstance(temp_enum['values'][0], str):
                raise EnumException('Enum autoId is false defined but values are string only')
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
            raise GroupException("Group name '{}' already defined, please check your model".format(temp_group['name']))
        check_names[temp_group['name']] = None
        if temp_group['persistence'] not in SUPPORTED_PERSISTENCE:
            raise GroupException("Persistence type '{}' is not supported".format(temp_group['persistence']))
        if temp_group['baseId'] in base_id:
            raise GroupException("Group baseId '{}' already defined, please check your model".format(temp_group['baseId']))
        base_id.append(temp_group['baseId'])
    return group_data


def struct_validator(struct_data):
    """
    Check the given struct data for consistency, duplicated and autoId.

    :param struct_data: dictionary of all struct definitions
    :return: given struct_data
    """
    check_names = dict()
    for temp_group in struct_data:
        if temp_group['name'] in check_names:
            raise StructException("Struct name '{}' already defined, please check your model".format(temp_group['name']))
        check_names[temp_group['name']] = None
        parameter_names = list()
        if len(temp_group['parameter']) == 0:
            raise StructException("Struct '{}' doesn't have any parameter defined".format(temp_group['name']))
        for temp_parameter in temp_group['parameter']:
            if temp_parameter['name'] in parameter_names:
                raise StructException("Struct name '{}' already defined, please check your model".format(temp_group['name']))
            parameter_names.append(temp_parameter['name'])
            if temp_parameter['type'] not in BASE_TYPES:
                raise StructException("Parameter type '{}' is not supported".format(temp_parameter['type']))
    return struct_data, check_names


def data_point_validator(data_point_data, struct_list):
    """
    Check the given data point for consistency and if given struct is defined.

    :param struct_list: dictionary of all struct definitions
    :param data_point_data: dictionary of all data points definitions
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
        if name in check_names:
            raise DatapointException(f"Datapoint name '{name}' already defined, please check your model")
        check_names[name] = None
        if access not in BASE_ACCESS:
            raise DatapointException(f"Datapoint access type '{access}' is not supported")
        if dp_type not in BASE_TYPES and dp_type not in struct_list:
            raise DatapointException(f"Datapoint type '{dp_type}' is not supported")
        if group not in group_id:
            group_id[group] = list()
        if group in group_id and dp_id in group_id[group]:
            raise DatapointException(f"Datapoint id '{dp_id}' is already registered for group '{group}'")
        group_id[group].append(dp_id)
    return data_point_data
