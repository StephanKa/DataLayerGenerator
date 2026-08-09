"""Compare two validated data-layer models for compatibility."""
import argparse
import json
from pathlib import Path

from generator import load_validated_model
from modelMetadata import create_model_metadata


def _access_capabilities(access):
    """Return the read and write capabilities represented by an access tag."""
    return {'read': access != 'WRITE_ONLY', 'write': access != 'READ_ONLY'}


def create_compatibility_report(previous, current):
    """Return a compatibility report from two machine-readable model catalogs."""
    breaking_changes = []
    compatible_changes = []
    previous_groups = {group['name']: group for group in previous['groups']}
    current_groups = {group['name']: group for group in current['groups']}
    previous_datapoints = {data_point['qualifiedName']: data_point for data_point in previous['datapoints']}
    current_datapoints = {data_point['qualifiedName']: data_point for data_point in current['datapoints']}
    current_ids = {data_point['globalId'] for data_point in current['datapoints']}
    current_legacy_ids = {legacy_id for data_point in current['datapoints'] for legacy_id in data_point['legacyGlobalIds']}

    for name, previous_group in previous_groups.items():
        current_group = current_groups.get(name)
        if current_group is None:
            breaking_changes.append({'kind': 'groupRemoved', 'group': name})
        elif previous_group['baseId'] != current_group['baseId']:
            breaking_changes.append({
                'kind': 'groupBaseIdChanged', 'group': name,
                'previous': previous_group['baseId'], 'current': current_group['baseId'],
            })
    for name in current_groups.keys() - previous_groups.keys():
        compatible_changes.append({'kind': 'groupAdded', 'group': name})

    for name, previous_data_point in previous_datapoints.items():
        current_data_point = current_datapoints.get(name)
        if current_data_point is None:
            if previous_data_point['globalId'] in current_ids | current_legacy_ids:
                compatible_changes.append({'kind': 'datapointIdRetained', 'datapoint': name})
            else:
                breaking_changes.append({'kind': 'datapointRemoved', 'datapoint': name})
            continue

        if (previous_data_point['globalId'] != current_data_point['globalId']
                and previous_data_point['globalId'] not in current_data_point['legacyGlobalIds']):
            breaking_changes.append({
                'kind': 'datapointIdChanged', 'datapoint': name,
                'previous': previous_data_point['globalId'], 'current': current_data_point['globalId'],
            })
        if previous_data_point['type'] != current_data_point['type']:
            breaking_changes.append({
                'kind': 'datapointTypeChanged', 'datapoint': name,
                'previous': previous_data_point['type'], 'current': current_data_point['type'],
            })

        previous_capabilities = _access_capabilities(previous_data_point['access'])
        current_capabilities = _access_capabilities(current_data_point['access'])
        for capability in ('read', 'write'):
            if previous_capabilities[capability] and not current_capabilities[capability]:
                breaking_changes.append({'kind': f'datapoint{capability.title()}AccessRemoved', 'datapoint': name})
        if previous_data_point['version'] != current_data_point['version']:
            compatible_changes.append({
                'kind': 'datapointVersionChanged', 'datapoint': name,
                'previous': previous_data_point['version'], 'current': current_data_point['version'],
            })

    for name in current_datapoints.keys() - previous_datapoints.keys():
        if current_datapoints[name]['globalId'] not in {data_point['globalId'] for data_point in previous_datapoints.values()}:
            compatible_changes.append({'kind': 'datapointAdded', 'datapoint': name})

    return {
        'formatVersion': 1,
        'compatible': not breaking_changes,
        'previousSummary': previous['summary'],
        'currentSummary': current['summary'],
        'breakingChanges': breaking_changes,
        'compatibleChanges': compatible_changes,
    }


def load_metadata(model_dir, schema_dir):
    """Load a model through the normal validation pipeline and return its catalog."""
    model = load_validated_model(model_dir, schema_dir)
    return create_model_metadata(**model)


def main():
    """Write a JSON compatibility report for a current and baseline model."""
    parser = argparse.ArgumentParser(description='Compare two DataLayerGenerator models.')
    parser.add_argument('--previous-model', required=True, help='Baseline model directory.')
    parser.add_argument('--current-model', required=True, help='Current model directory.')
    parser.add_argument('--schema-dir', required=True, help='Directory containing schema.json.')
    parser.add_argument('--output', required=True, help='Path for the JSON report.')
    args = parser.parse_args()

    report = create_compatibility_report(
        load_metadata(args.previous_model, args.schema_dir),
        load_metadata(args.current_model, args.schema_dir),
    )
    with Path(args.output).open('w', encoding='utf-8') as output:
        json.dump(report, output, indent=2)


if __name__ == '__main__':
    main()
