"""This module will generate a CSV file for all datapoints."""


class Overview:
    """This class generate a table for all datapoints and ID's."""

    def __init__(self, group_ids, datapoint_id):
        """Initialize overview class."""
        self.group_ids = group_ids
        self.datapoint_id = datapoint_id
        self.data = [f'Name;Group;Id;Namespace;Type;Version;Description']
        self.__generate_data()

    def __generate_data(self):
        """Create data list with datapoint, id and group name."""
        for dp in self.datapoint_id:
            temp_id = self.group_ids[dp['group']] + dp['id']
            description = ''
            namespace = ''
            if 'description' in dp and dp['description'] is not None:
                description = dp['description']
            if 'namespace' in dp:
                namespace = dp['namespace']
            self.data.append(f'{dp["name"]};{dp["group"]};{temp_id:#x};{namespace};{dp["type"]};{dp["version"]};'
                             f'{description}')

    def __str__(self):
        """Return a string of UML content."""
        return '\n'.join(self.data)


def generate_overview(path, group_ids, data_point_id):
    """General method to be called from outside."""
    temp = Overview(group_ids, data_point_id)
    with open(f'{path}/overview.csv', 'w') as file:
        file.write(str(temp))
