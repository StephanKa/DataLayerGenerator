"""This generator will validate json defined JSON as model via a schema and generate according C++."""
import json
import os
import jsonschema
import argparse
from jinja2 import Environment, FileSystemLoader
from validators import enum_validator, struct_validator, group_validator, data_point_validator


def validate_json(model_data, schema):
    """
    Validate the given model json with defined schema.

    :param model_data: json model file content
    :param schema: json schema file content
    :return:
    """
    jsonschema.validate(instance=model_data, schema=schema)


def create_group_data_point_dict(dps):
    """
    Create a dictionary which will have group and data points belong to this group.

    :param dps: list of all data points
    :return: group data point mapping.
    """
    temp = dict()
    for dp in dps:
        group = dp['group']
        name = dp['name']
        if group not in temp:
            temp[group] = []
        if 'namespace' in dp:
            name = '{}::{}'.format(dp['namespace'], name)
        temp[group].append(name)
    return temp


def get_args():
    """
    Parse defined args and check for existence.

    :return: parsed args
    """
    parser = argparse.ArgumentParser(description='Generates code from defined model.')
    parser.add_argument('-s', '--source_dir', required=True, help='Source directory which point to the generator')
    parser.add_argument('-o', '--out_dir', required=True, help='Out directory which point to the folder where the generated files will be saved.')
    return parser.parse_args()


def main(template_file_name):
    """
    Execute all work for parsing and validating.

    :param template_file_name: defines the file that shall be rendered.
    """
    args = get_args()
    with open(f'{args.source_dir}/generator/schema.json') as f:
        datalayer_schema = json.load(f)

    file_loader = FileSystemLoader(f'{args.source_dir}/template')
    env = Environment(loader=file_loader)

    with open(f'{args.source_dir}/model/model.json') as file:
        data = file.read()
    json_data = json.loads(data)

    # validate json scheme
    validate_json(json_data, datalayer_schema)

    # validate all defined sections
    enums = enum_validator(json_data['Enums'])
    groups = group_validator(json_data['Groups'])
    structs, struct_names = struct_validator(json_data['Structs'])
    data_points = data_point_validator(json_data['Datapoints'], struct_names)

    group_data_points_mapping = create_group_data_point_dict(data_points)

    template = env.get_template(template_file_name)
    output = template.render(enums=enums, groups=groups, structs=structs, data_points=data_points, group_data_points_mapping=group_data_points_mapping)

    if not os.path.exists(f'{args.out_dir}/generated'):
        os.mkdir(f'{args.out_dir}/generated')
    with open(f'{args.out_dir}/generated/datalayer.h', 'w') as f:
        f.write(output)


if __name__ == '__main__':
    main('datalayer.h.jinja2')
