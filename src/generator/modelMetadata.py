"""Create machine-readable metadata from validated data-layer models."""
import hashlib
import json


def _version_string(version):
    """Return a semantic version string from a validated version value."""
    return f'{version.major}.{version.minor}.{version.build}'


def _qualified_name(data_point):
    """Return a datapoint's C++ name including its optional namespace."""
    if data_point['namespace']:
        return f"{data_point['namespace']}::{data_point['name']}"
    return data_point['name']


def _enum_values(values):
    """Convert validated enum values into JSON-serializable values."""
    result = []
    for value in values:
        if isinstance(value, str):
            result.append(value)
        else:
            result.append({'name': value.name, 'value': value.value})
    return result


def create_model_fingerprint(metadata):
    """Return a deterministic 64-bit fingerprint for model compatibility data."""
    datapoints = [
        {**data_point, 'legacyGlobalIds': sorted(data_point.get('legacyGlobalIds', []))}
        for data_point in metadata['datapoints']
    ]
    enums = []
    for enum in metadata['enums']:
        values = enum['values']
        if not enum['autoId']:
            values = sorted(values, key=lambda value: (value['value'], value['name']))
        enums.append({**enum, 'values': values})
    canonical_metadata = {
        'summary': metadata['summary'],
        'groups': sorted(metadata['groups'], key=lambda group: group['name']),
        'datapoints': sorted(datapoints, key=lambda data_point: data_point['qualifiedName']),
        'enums': sorted(enums, key=lambda enum: enum['name']),
        'structs': sorted(metadata['structs'], key=lambda struct: struct['name']),
        'types': sorted(metadata['types'], key=lambda data_type: data_type['name']),
    }
    encoded_metadata = json.dumps(canonical_metadata, sort_keys=True, separators=(',', ':'), ensure_ascii=True).encode('utf-8')
    return int.from_bytes(hashlib.sha256(encoded_metadata).digest()[:8], byteorder='big')


def create_model_metadata(enums, groups, structs, data_points, types):
    """Return a JSON-serializable catalog for a validated data-layer model."""
    group_ids = {group['name']: int(group['baseId'], 0) for group in groups}
    access_counts = {access: 0 for access in ('READ_ONLY', 'WRITE_ONLY', 'READ_WRITE')}
    metadata_data_points = []

    for data_point in data_points:
        access_counts[data_point['access']] += 1
        group_id = group_ids[data_point['group']]
        metadata_data_points.append({
            'name': data_point['name'],
            'qualifiedName': _qualified_name(data_point),
            'group': data_point['group'],
            'id': data_point['id'],
            'globalId': group_id + data_point['id'],
            'legacyGlobalIds': [group_id + legacy_id for legacy_id in data_point['renamedFrom']],
            'type': data_point['type'],
            'access': data_point['access'],
            'arraySize': data_point['arraySize'],
            'version': _version_string(data_point['version']),
            'allowUpgrade': data_point['allowUpgrade'] == 'true',
            'description': data_point['description'],
        })

    metadata = {
        'formatVersion': 1,
        'summary': {
            'groups': len(groups),
            'datapoints': len(data_points),
            'structs': len(structs),
            'enums': len(enums),
            'types': len(types),
            'datapointsByAccess': access_counts,
        },
        'groups': [{
            'name': group['name'],
            'baseId': group_ids[group['name']],
            'version': _version_string(group['version']),
            'persistence': group['persistence'],
            'allowUpgrade': group['allowUpgrade'] == 'true',
            'maxPayloadBytes': group['maxPayloadBytes'],
            'description': group['description'],
        } for group in groups],
        'datapoints': metadata_data_points,
        'enums': [{
            'name': enum['name'],
            'type': enum['type'],
            'autoId': enum.get('autoId', False),
            'values': _enum_values(enum['values']),
        } for enum in enums],
        'structs': [{
            'name': struct['name'],
            'members': [{'name': parameter.name, 'type': parameter.value} for parameter in struct['parameter']],
        } for struct in structs],
        'types': [{
            'name': data_type['name'],
            'type': data_type['type'],
            'minimum': data_type['min'],
            'maximum': data_type['max'],
            'step': data_type['step'],
            'allowedValues': data_type['allowedValues'],
            'minimumLength': data_type['minLength'],
            'maximumLength': data_type['maxLength'],
            'pattern': data_type['pattern'],
        } for data_type in types],
    }
    fingerprint = create_model_fingerprint(metadata)
    metadata['modelFingerprint'] = {
        'algorithm': 'sha256-64',
        'value': f'{fingerprint:016x}',
    }
    return metadata
