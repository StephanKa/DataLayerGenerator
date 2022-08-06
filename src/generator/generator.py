""" This generator will validate json defined JSON as model via a schema and generate according C++ """
import json
import os
from jinja2 import Environment, FileSystemLoader
import jsonschema
import argparse


BASE_TYPES = ["uint8_t", "uint16_t", "uint32_t", "int8_t", "int16_t", "int32_t", "float", "double"]
SUPPORTED_PERSISTENCE = ["None", "Cyclic", "OnWrite"]
BASE_ACCESS = ["READACCESS", "WRITEACCESS", "READWRITE"]


def validate_json(model_data, schema):
    """ validate the given model json with defined schema

    :param model_data: json model file content
    :param schema: json schema file content
    :return:
    """
    jsonschema.validate(instance=model_data, schema=schema)


class EnumException(Exception):
    """ User defined exception for faulty enum definitions """
    pass


class GroupException(Exception):
    """ User defined exception for faulty group definitions """
    pass


class StructException(Exception):
    """ User defined exception for faulty struct definitions """
    pass


class DatapointException(Exception):
    """ User defined exception for faulty data points definitions """
    pass


def enum_validator(enum_data):
    """ Checks given enum data for consistency, duplicated and autoId

    :param enum_data: dictionary of all enum definitions
    :return: given enum_data
    """
    check_names = dict()
    for temp_enum in enum_data:
        if temp_enum["name"] in check_names:
            raise EnumException("Enum name '{0}' already defined, please check your model".format(temp_enum["name"]))
        check_names[temp_enum["name"]] = None
        if temp_enum["type"] not in BASE_TYPES:
            raise EnumException("Enum type '{}' is not supported".format(temp_enum["type"]))
        if temp_enum["autoId"] and len(temp_enum["values"]) > 0 and not isinstance(temp_enum["values"][0], str):
            raise EnumException("Enum autoId is true but values are not string only")
        elif not temp_enum["autoId"] and len(temp_enum["values"]) > 0 and isinstance(temp_enum["values"][0], str):
            raise EnumException("Enum autoId is false defined but values are string only")
    return enum_data


def group_validator(group_data):
    """ Checks given group data for consistency, duplicated and autoId

    :param group_data: dictionary of all group definitions
    :return: given group_data
    """
    check_names = dict()
    base_id = dict()
    for temp_group in group_data:
        if temp_group["name"] in check_names:
            raise GroupException("Group name '{0}' already defined, please check your model".format(temp_group["name"]))
        check_names[temp_group["name"]] = None
        if temp_group["persistence"] not in SUPPORTED_PERSISTENCE:
            raise EnumException("Persistence type '{}' is not supported".format(temp_group["persistence"]))
        if temp_group["baseId"] in check_names:
            raise GroupException("Group baseId '{0}' already defined, please check your model".format(temp_group["baseId"]))
        base_id[temp_group["baseId"]] = None
    return group_data


def struct_validator(struct_data):
    """ Checks given struct data for consistency, duplicated and autoId

    :param struct_data: dictionary of all struct definitions
    :return: given struct_data
    """
    check_names = dict()
    for temp_group in struct_data:
        if temp_group["name"] in check_names:
            raise StructException("Struct name '{0}' already defined, please check your model".format(temp_group["name"]))
        check_names[temp_group["name"]] = None
        parameter_names = list()
        for temp_parameter in temp_group["parameter"]:
            if temp_parameter["name"] in parameter_names:
                raise StructException("Struct name '{0}' already defined, please check your model".format(temp_group["name"]))
            parameter_names.append(temp_parameter["name"])
            if temp_parameter["type"] not in BASE_TYPES:
                raise StructException("Parameter type '{}' is not supported".format(temp_parameter["type"]))
    return struct_data, check_names


def data_point_validator(data_point_data, struct_list):
    """ Checks given data point for consistency and if given struct is defined

    :param struct_list: dictionary of all struct definitions
    :param data_point_data: dictionary of all data points definitions
    :return: given data_point_data
    """
    check_names = dict()
    group_id = dict()
    for temp_dp in data_point_data:
        group = temp_dp["group"]
        name = temp_dp["name"]
        access = temp_dp["access"]
        dp_id = temp_dp["id"]
        dp_type = temp_dp["type"]
        if "namespace" in temp_dp:
            name = "{0}::{1}".format(temp_dp["namespace"], name)
        else:
            temp_dp["namespace"] = ""
        if name in check_names:
            raise DatapointException("Datapoint name '{0}' already defined, please check your model".format(name))
        check_names[name] = None
        if access not in BASE_ACCESS:
            raise DatapointException("Datapoint access type '{}' is not supported".format(access))
        if dp_type not in BASE_TYPES and dp_type not in struct_list:
            raise DatapointException("Datapoint type '{}' is not supported".format(dp_type))
        if group not in group_id:
            group_id[group] = list()
        if group in group_id and dp_id in group_id[group]:
            raise DatapointException("Datapoint id '{}' is already registered for group '{}'".format(dp_id, group))
        group_id[group].append(dp_id)
    return data_point_data


def create_group_data_point_dict(dps):
    """ Creates a dictionary which will have group and data points belong to this group.

    :param dps: list of all data points
    :return: group data point mapping.
    """
    temp = dict()
    for dp in dps:
        group = dp["group"]
        name = dp["name"]
        if group not in temp:
            temp[group] = []
        if "namespace" in dp:
            name = "{0}::{1}".format(dp["namespace"], name)
        temp[group].append(name)
    return temp


def get_args():
    """ Parse defined args and check for existence.

    :return: parsed args
    """
    parser = argparse.ArgumentParser(description='Generates code from defined model.')
    parser.add_argument('-s', '--source_dir', required=True, help='Source directory which point to the generator')
    parser.add_argument('-o', '--out_dir', required=True, help='Out directory which point to the folder where the generated files will be saved.')
    return parser.parse_args()


if __name__ == '__main__':
    args = get_args()
    with open('{}/generator/schema.json'.format(args.source_dir), "r") as f:
        datalayer_schema = json.load(f)

    file_loader = FileSystemLoader('{}/template'.format(args.source_dir))
    env = Environment(loader=file_loader)

    with open("{}/model/model.json".format(args.source_dir)) as file:
        data = file.read()
    json_data = json.loads(data)

    # validate json scheme
    validate_json(json_data, datalayer_schema)

    # validate all defined sections
    enums = enum_validator(json_data["Enums"])
    groups = group_validator(json_data["Groups"])
    structs, struct_names = struct_validator(json_data["Structs"])
    data_points = data_point_validator(json_data["Datapoints"], struct_names)

    group_data_points_mapping = create_group_data_point_dict(data_points)

    template = env.get_template('datalayer.h.jinja2')
    output = template.render(enums=enums, groups=groups, structs=structs, data_points=data_points, group_data_points_mapping=group_data_points_mapping)

    if not os.path.exists("{0}/generated".format(args.out_dir)):
        os.mkdir("{0}/generated".format(args.out_dir))
    with open("{0}/generated/datalayer.h".format(args.out_dir), "w") as f:
        f.write(output)
