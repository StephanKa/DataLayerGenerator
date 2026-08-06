"""This module generates PlantUML diagrams for a validated data-layer model."""
from collections import defaultdict
from pathlib import Path
import re


def plantuml_alias(prefix, name):
    """Return a stable PlantUML-safe alias for a model element."""
    return f'{prefix}_{re.sub(r"[^A-Za-z0-9_]", "_", name)}'


def plantuml_label(value):
    """Return text safely quoted for use as a PlantUML display label."""
    return str(value).replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')


def generate_enumeration(enums):
    """
    General method that will create a list of string in UML format.

    :param enums: validated enumeration definitions
    :return: list of UML enumeration strings
    """
    result = []
    for enum in enums:
        members = []
        for value in enum['values']:
            if isinstance(value, str):
                members.append(value)
            else:
                members.append(f'{value.name} = {value.value}')
        alias = plantuml_alias('enum', enum['name'])
        result.append(f'enum "{plantuml_label(enum["name"])}" as {alias} {{\n'
                      f'{chr(10).join(members)}\n}}')
    return result


def generate_struct(structs):
    """
    General method that will create a list of string in UML format.

    :param structs: validated struct definitions
    :return: list of UML struct / classes strings
    """
    result = []
    for struct in structs:
        attributes = '\n'.join(f'{parameter.name}: {parameter.value}' for parameter in struct['parameter'])
        alias = plantuml_alias('struct', struct['name'])
        result.append(f'class "{plantuml_label(struct["name"])}" as {alias} <<data type>> {{\n'
                      f'{attributes}\n}}')
    return result


def generate_types(types):
    """Generate PlantUML declarations for custom alias types."""
    result = []
    for custom_type in types:
        alias = plantuml_alias('type', custom_type['name'])
        result.append(f'class "{plantuml_label(custom_type["name"])}" as {alias} <<alias>> {{\n'
                      f'type: {custom_type["type"]}\n}}')
    return result


def type_alias(type_name, enum_names, struct_names, custom_type_names):
    """Return the PlantUML alias for a named model type, when one exists."""
    if type_name in enum_names:
        return plantuml_alias('enum', type_name)
    if type_name in struct_names:
        return plantuml_alias('struct', type_name)
    if type_name in custom_type_names:
        return plantuml_alias('type', type_name)
    return None


def generate_datapoints(datapoints, enum_names, struct_names, custom_type_names):
    """
    General method that will create a list of string in UML format.

    :param datapoints: validated datapoint definitions
    :param enum_names: names of defined enums
    :param struct_names: names of defined structs
    :param custom_type_names: names of defined alias types
    :return: tuple of UML datapoint content and relationships
    """
    groups = defaultdict(lambda: defaultdict(list))
    relationships = []
    for datapoint in datapoints:
        namespace = datapoint['namespace']
        alias = plantuml_alias('datapoint', f'{datapoint["group"]}_{namespace}_{datapoint["name"]}')
        type_display = datapoint['type']
        if datapoint['arraySize']:
            type_display += f'[{datapoint["arraySize"]}]'
        attributes = [
            f'type: {type_display}',
            f'access: {datapoint["access"]}',
            f'id: {datapoint["id"]:#x}',
            f'version: {datapoint["version"]}',
        ]
        if datapoint['description'] is not None:
            attributes.append(f'description: {plantuml_label(datapoint["description"])}')
        groups[datapoint['group']][namespace].append(
            f'class "{plantuml_label(datapoint["name"])}" as {alias} {{\n'
            f'{chr(10).join(attributes)}\n}}')

        target_alias = type_alias(datapoint['type'], enum_names, struct_names, custom_type_names)
        if target_alias is not None:
            relationships.append(f'{alias} *-- {target_alias}')

    packages = []
    for group, namespaces in groups.items():
        members = []
        for namespace, datapoint_classes in namespaces.items():
            content = '\n'.join(datapoint_classes)
            if namespace:
                members.append(f'package "{plantuml_label(namespace)}" {{\n{content}\n}}')
            else:
                members.append(content)
        packages.append(f'package "{plantuml_label(group)}" #DDDDDD {{\n'
                        f'{chr(10).join(members)}\n}}')
    return '\n'.join(packages), relationships


class UML:
    """This class generates UML strings."""

    START = '@startuml\n'
    END = '@enduml'

    def __init__(self, enums, structs, datapoints, types):
        """Initialize UML class."""
        enum_names = {enum['name'] for enum in enums}
        struct_names = {struct['name'] for struct in structs}
        custom_type_names = {custom_type['name'] for custom_type in types}
        self.enum_string = generate_enumeration(enums)
        self.struct_string = generate_struct(structs)
        self.type_string = generate_types(types)
        self.datapoint_string, self.relationships = generate_datapoints(
            datapoints, enum_names, struct_names, custom_type_names)
        self.relationships.extend(
            f'{plantuml_alias("struct", struct["name"])} *-- '
            f'{plantuml_alias("struct", parameter.value)}'
            for struct in structs
            for parameter in struct['parameter']
            if parameter.value in struct_names
        )
        self.relationships = list(dict.fromkeys(self.relationships))

    def __str__(self):
        """Return a string of UML content."""
        sections = [
            '\n'.join(self.enum_string),
            '\n'.join(self.struct_string),
            '\n'.join(self.type_string),
            self.datapoint_string,
            '\n'.join(self.relationships),
        ]
        return self.START + '\n\n'.join(section for section in sections if section) + '\n' + self.END


def generate_uml(enums, structs, datapoints, types, out_dir):
    """
    General method that will create a list of string in UML format.

    :param enums: validated enum definitions
    :param structs: validated struct definitions
    :param datapoints: validated datapoint definitions
    :param types: validated custom type definitions
    :param out_dir: output directory path
    """
    output_path = Path(out_dir) / 'overview.plantuml'
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(str(UML(enums, structs, datapoints, types)), encoding='utf-8')
