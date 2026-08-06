# Agent Guide

## Scope

DataLayerGenerator is a header-only C++23 framework and a Python/Jinja2 model generator. Keep changes narrow and preserve its embedded-friendly properties: generated globals, compile-time access control, and no required runtime dependencies.

## Repository Map

- `src/include/`: framework headers shipped to consumers.
- `src/generator/`: model loading, JSON Schema validation, semantic validation, and the local Flask model editor.
- `src/generator/templates/` and `src/generator/static/`: Bootstrap editor page, styling, and browser behavior.
- `src/template/`: Jinja2 templates for generated C++ headers and bindings.
- `src/model/`: example input model.
- `test/catch2/`: generated-model C++ tests.
- `test/python/`: generator and validator tests.
- `docs/`: Sphinx documentation.
- `cmake/`: presets, toolchains, and project helpers.

## Change Rules

- Treat a model field as a public API. Update `schema.json`, `validators.py`, templates, tests, and `docs/model.rst` together when adding or changing one.
- Keep generated C++ compatible with the configured C++23 standard and use existing framework patterns.
- Preserve `USE_FILE_PERSISTENCE`, `USE_FMT`, and other feature gates. Do not impose MQTT, CAN, Python, or `fmt` dependencies on the base headers.
- Persistence changes need tests for malformed input, corruption, version handling, and recovery behavior.
- For generated output changes, exercise both the Python generator and a representative Catch2 target.
- The local model editor writes only the canonical JSON layout: `groups.json`, `datapoints.json`, `structs.json`, `enums.json`, and `types.json`. Keep validation-before-write and atomic writes; reject YAML or custom split-file layouts rather than risking duplicate definitions.
- Keep the editor bound to loopback by default. Treat any non-loopback `--host` as an explicit user choice because the local API has no authentication.
- Do not edit generated files in `build/` or `cmake-build-*`; change source, templates, or model inputs instead.

## Style and Validation

- C++ formatting is enforced by pre-commit using `.clang-format`.
- Python changes must pass `pydocstyle`, `pyupgrade`, and JSON/schema checks in pre-commit.
- Run focused checks first, then the relevant CMake test preset. See `build.md` for commands.
- For model-editor changes, run `.venv\Scripts\python.exe test\python\testModelEditor.py` and verify the browser UI against a canonical JSON model directory.
- Update `README.md` and the relevant Sphinx page for user-visible APIs, model fields, build options, or integration boundaries.

## Git

- Keep commits focused by topic: framework/generator behavior, adapters, tests, and documentation should be separate when practical.
- Do not commit generated build artifacts or unrelated formatting changes.
