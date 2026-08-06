# Build Guide

## Prerequisites

- CMake 3.25 or later
- A C++23-capable compiler
- Python 3.9 or later with `Flask`, `jinja2`, `jsonschema`, and `PyYAML`
- Conan 2.x for C++ dependencies

Install ``uv`` and create the project virtual environment:

```powershell
uv venv
uv pip install --python .venv\Scripts\python.exe -r src\generator\requirements.txt
```

Install project dependencies before configuring a normal host build:

```powershell
conan install . --build=missing
```

## Install And Consume

Install the CMake package without examples or tests:

```powershell
cmake -S . -B build/install -DBUILD_EXAMPLES=OFF -DENABLE_TESTING=OFF
cmake --install build/install --prefix C:/local/DataLayerGenerator
```

Consumer projects can locate it and link the header-only target:

```cmake
find_package(DataLayerGenerator CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE DataLayerGenerator::DataLayerGenerator)
```

Create a Conan package from the repository with:

```powershell
conan create . --build=missing
```

The Conan options `with_fmt` and `with_file_persistence` control the matching
`USE_FMT` and `USE_FILE_PERSISTENCE` compile definitions exported to consumers.

## List Presets

```powershell
cmake --list-presets
cmake --build --list-presets
ctest --list-presets
```

Top-level presets include GCC, Clang, Windows/MSVC or MinGW, and ARM Cortex-M4 configurations. Use a preset that matches an installed compiler.

## Host Build

Linux GCC example:

```bash
cmake --preset gcc-14-debug
cmake --build --preset gcc-14-debug
ctest --preset gcc-14-debug --output-on-failure
```

Windows MSVC example:

```powershell
cmake --preset windows-2022-debug
cmake --build --preset windows-2022-debug
ctest --preset windows-2022-debug --output-on-failure
```

To build a single Catch2 target after configuration:

```powershell
cmake --build build/debug/windows-2022-debug --target datapoints_tests
ctest --test-dir build/debug/windows-2022-debug --output-on-failure -R "^datapoints\."
```

Adjust the build directory to the selected preset. `cmake --preset <name>` prints the configured binary directory.

## Generator Checks

Validate a model without writing generated files:

```powershell
python src/generator/generator.py `
  --model_dir src/model `
  --out_dir build `
  --template_dir src/template `
  --schema_dir src/generator `
  --check
```

Generate artifacts directly outside CMake:

```powershell
python src/generator/generator.py `
  --model_dir src/model `
  --out_dir build `
  --template_dir src/template `
  --schema_dir src/generator `
  --module_name datalayer_example
```

## Local Model Editor

The optional Flask editor manages groups, datapoints, structs, enums, and alias types in a local
browser UI. It uses the installed generator requirements and binds to `127.0.0.1` by default:

```powershell
.\.venv\Scripts\python.exe src\generator\model_editor.py --model-dir src\model
```

Open `http://127.0.0.1:5000`. Use `--port <port>` to select another port. The editor accepts and
writes only the canonical JSON layout: `groups.json`, `datapoints.json`, `structs.json`,
`enums.json`, and `types.json`. Use the generator directly for YAML or custom split-file models.

Run the focused editor API tests with:

```powershell
.\.venv\Scripts\python.exe test\python\testModelEditor.py
```

## Python Wheel

Build the generated Python extension as a wheel with CMake as the single build authority:

```powershell
uv pip install --python .venv\Scripts\python.exe build
Push-Location datalayer_example
uv build
uv pip install --python ..\.venv\Scripts\python.exe --force-reinstall .\dist\datalayer_example-0.0.1-*.whl
Pop-Location
```

The wheel configures CMake with ``ENABLE_PYBIND11=ON`` and installs the generated extension. Use
``-DPYBIND11_DISABLE_EXTRAS=ON`` for faster development builds when configuring CMake directly.

After installation, the generated module exposes model structs and static datapoint accessors:

```python
import datalayer_example as dl

dl.Test.set(42)
print(dl.Test.get())

temperature = dl.Temperature(2350, 23.5)
dl.Test4.set(temperature)

dl.Arraytest2.set([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
print(dl.Arraytest2.get())
```

Generated names are based on the datapoints in the input model. Use ``dir(dl)`` to inspect a custom
model's extension. Set ``-DPYBIND11_MODULE_NAME=my_datalayer`` when configuring CMake to select a
different import name.

## Feature Options

Pass feature options while configuring, or define them in a CMake user preset:

```powershell
cmake --preset windows-2022-debug `
  -DENABLE_FILE_PERSISTENCE=ON `
  -DENABLE_FMT=ON `
  -DENABLE_PYBIND11=ON
```

Common options are `ENABLE_TESTING`, `ENABLE_DOCS`, `ENABLE_FILE_PERSISTENCE`, `ENABLE_FMT`, `ENABLE_PYBIND11`, `ENABLE_FUZZING`, and the sanitizer options in `cmake/Options.cmake`.

## ARM Cross Build

Configure the ARM preset after installing the required Cortex-M GCC toolchain:

```bash
cmake --preset gcc-arm-release
cmake --build --preset gcc-arm-release
```

Cross builds do not build or run the host Catch2 tests.

## Documentation

The CMake documentation target uses Sphinx and Doxygen. Install the Python packages required by the docs:

```powershell
uv pip install --python .venv\Scripts\python.exe sphinx sphinx-rtd-theme breathe sphinx-copybutton
```

For a standalone documentation check, first configure a docs build to generate the Doxygen XML, then build the Sphinx target supplied by CMake. A source-only syntax/link check can be run with:

```powershell
sphinx-build -W -b dummy docs build/docs-check
```

## Formatting and Pre-commit

Install the repository hooks once:

```powershell
pre-commit install
```

Run all checks before committing:

```powershell
pre-commit run --all-files
```

The hooks format C++ with `clang-format`, validate Python and JSON, and run Python style upgrades. Re-stage any files modified by the hooks before committing.
