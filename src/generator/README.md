# DataLayerGenerator Python Tools

This directory contains the JSON/YAML model generator, semantic validators, and the local Flask
model editor. The canonical model reference is in `../../docs/model.rst`; the project build guide is
in `../../build.md`.

## Install

From the repository root, install the declared Python dependencies:

```powershell
uv venv
uv pip install --python .venv\Scripts\python.exe -r src\generator\requirements.txt
```

## Generate Or Validate

Validate a model without writing generated files:

```powershell
.\.venv\Scripts\python.exe src\generator\generator.py `
  --model_dir src\model `
  --out_dir build `
  --template_dir src\template `
  --schema_dir src\generator `
  --check
```

Remove `--check` to generate C++ headers, formatter support, optional Python bindings, and model
documentation. Model directories can contain JSON, YAML, or a mix of both; the generator merges all
supported files before schema and semantic validation.

## Local Model Editor

Start the browser editor for a canonical JSON model directory:

```powershell
.\.venv\Scripts\python.exe src\generator\model_editor.py --model-dir src\model
```

Open `http://127.0.0.1:5000`. The editor validates changes before saving and writes atomically, but
it intentionally supports only `groups.json`, `datapoints.json`, `structs.json`, `enums.json`, and
`types.json`. Use `generator.py` directly for YAML or custom split-file layouts.

## Tests

Run the local editor API tests with:

```powershell
.\.venv\Scripts\python.exe test\python\testModelEditor.py
```

Run the remaining generator and validator tests through the project's configured test workflow in
`../../build.md`.
