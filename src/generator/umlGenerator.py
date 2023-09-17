"""This module will generate UML diagrams for all datapoints."""
from constants import BASE_TYPES


def generate_enumeration(enum_dict):
    """
    General method that will create a list of string in UML format.

    :param enum_dict: dictionary of all enumeration definitions
    :return: list of UML enumeration strings
    """
    result = []
    for data in enum_dict:
        enum_string = f'enum {data["name"]} {{\n'
        if len(data['values']) == 0:
            continue
        if isinstance(data['values'][0], str):
            enum_string += '\n'.join(data['values'])
        if isinstance(data['values'][0], dict):
            for temp in data['values']:
                (key, value), = temp.items()
                enum_string += f'{key} = {value}\n'
        enum_string += '\n}\n'
        result.append(enum_string)
    return result


def generate_struct(struct_dict):
    """
    General method that will create a list of string in UML format.

    :param struct_dict: dictionary of all struct definitions
    :return: list of UML struct / classes strings
    """
    result = []
    for data in struct_dict:
        struct_string = f'class {data["name"]} << data type >> {{\n'
        for temp in data['parameter']:
            struct_string += f'{temp.name} {temp.value}\n'
        struct_string += '}\n'
        result.append(struct_string)
    return result


def generate_datapoints(datapoint_dict):
    """
    General method that will create a list of string in UML format.

    :param datapoint_dict: dictionary of all datapoint definitions
    :return: tuple of lists of UML struct and relationsship
    """
    result = dict()
    relation_ship = []
    for data in datapoint_dict:
        namespace = ''
        group = data['group']
        if 'namespace' in data and data['namespace'] != '':
            namespace = f'package {data["namespace"]} {{ \n'
        dp_string = namespace + f'class {data["name"]} {{\n {data["type"]} \n}}\n'
        if namespace != '':
            dp_string += '\n}'
        if data['type'] not in BASE_TYPES:
            relation_ship.append(f'{data["name"]} *- {data["type"]}')

        if group in result:
            result[group].append(dp_string)
        else:
            result[group] = [dp_string]
    output = ''
    for group in result:
        temp = '\n'.join(result[group])
        output += f'package "{group}" #DDDDDD {{ \n {temp} \n}}\n'
    return output, relation_ship


class UML:
    """This class generates UML strings."""

    START = '@startuml\n'
    END = '@enduml'

    def __init__(self, enum_dict, struct_dict, datapoint_struct):
        """Initialize UML class."""
        self.enum_string = generate_enumeration(enum_dict)
        self.struct_string = generate_struct(struct_dict)
        self.datapoint_string, self.relation_ship = generate_datapoints(datapoint_struct)

    def __str__(self):
        """Return a string of UML content."""
        return self.START + '\n'.join(self.enum_string) + '\n' + '\n'.join(
            self.struct_string) + '\n' + self.datapoint_string + '\n'.join(self.relation_ship) + '\n' + self.END


def generate_uml(enums, structs, datapoint, out_dir):
    """
    General method that will create a list of string in UML format.

    :param enums: dictionary of all enum definitions
    :param structs: dictionary of all struct definitions
    :param datapoint: dictionary of all datapoint definitions
    :param out_dir: path for the generated output
    """
    d = UML(enums, structs, datapoint)
    with open(f'{out_dir}/datapoint_overview.plantuml', 'w') as e:
        e.write(str(d))
