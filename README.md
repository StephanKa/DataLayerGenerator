# DataLayerGenerator

[![CMake CI](https://github.com/StephanKa/DataLayerGenerator/actions/workflows/build_cmake.yml/badge.svg?branch=main)](https://github.com/StephanKa/DataLayerGenerator/actions/workflows/build_cmake.yml)
[![Documentation](https://github.com/StephanKa/DataLayerGenerator/actions/workflows/docs.yml/badge.svg?branch=main)](https://stephanKa.github.io/DataLayerGenerator/)

**📖 [Full documentation on GitHub Pages](https://StephanKa.github.io/DataLayerGenerator/)**

---

A **header-only C++20/23 framework** with a **Python/Jinja2 code generator** for building type-safe,
zero-allocation data layers — targeting embedded systems (MCU / bare-metal) as well as host
applications.

You describe your data model in JSON or YAML files. The generator produces a single `datalayer.h`
header that contains fully type-safe `constinit` datapoint globals, with access control enforced at
compile time, optional range checking, robust file persistence, change callbacks, optional `fmt`
formatters, optional Python bindings, and dependency-free MQTT/CAN adapter boundaries.

---

## Features

| Feature | Description |
|---|---|
| **C++20/23** | Requires C++20 minimum — leverages Concepts, NTTPs, `consteval`, `constinit`, `std::span`, `std::expected` |
| **Header-only framework** | `src/include/` has no compiled artifacts and no external dependencies |
| **Compile-time access control** | `READ_ONLY`, `WRITE_ONLY`, `READ_WRITE` enforced via C++20 Concepts |
| **Range checking** | Alias types carry `Minimum`/`Maximum`; violations return `RangeCheck::underflow/overflow` |
| **Versioning & migration** | Groups and datapoints carry versions; legacy IDs and byte-level migration callbacks support model evolution |
| **Persistence** | `DLG1` records include group identity, versions, lengths, CRC32, and staged replacement writes (`-DENABLE_FILE_PERSISTENCE=ON`) |
| **Change callbacks** | Optional allocation-free callback invoked after a successful datapoint write |
| **Transport boundaries** | Dependency-free typed MQTT and CAN/CAN-FD adapters; applications supply the broker client or CAN driver |
| **fmt support** | Auto-generated `fmt::formatter<>` for all custom structs and enums (`-DENABLE_FMT=ON`) |
| **Python bindings** | Auto-generated `pybind11` module (`-DENABLE_PYBIND11=ON`) |
| **JSON & YAML models** | Write model files in JSON or YAML; mixed directories supported |
| **Cross-compilation** | ARM Cortex-M4 GCC toolchain included |
| **Conan package** | Distributable as a Conan `header-library` package with consumer helper CMake function |

---

## Quick Start

### Prerequisites

- CMake ≥ 3.25
- C++23 compiler (GCC 13+, Clang 16+, MSVC 2022 17.5+)
- Python ≥ 3.9 and [uv](https://docs.astral.sh/uv/) for virtual environments and Python dependencies
- [Conan 2.x](https://conan.io/)

### Build

```bash
# Install dependencies via Conan and configure
conan install . --build=missing
cmake --preset gcc-14-release   # see CMakePresets.json for available presets

# Build
cmake --build --preset gcc-14-release

# Run tests
ctest --preset gcc-14-release
```

### Docker (alternative)

```bash
cd docker
./build-dev-image.sh   # developer image with all tools pre-installed
./build-ci-image.sh    # CI image
```

---

## Project Structure

```
src/
├── include/        ← Header-only C++ framework (ship this + generated header)
├── generator/      ← Python code generator (generator.py, validators, templates)
├── template/       ← Jinja2 templates (datalayer.h, formatter.h, pythonBinding.cpp)
├── model/          ← Example model files (JSON)
├── main.cpp        ← Host demo executable
└── embedded.cpp    ← Bare-metal demo executable

test/
├── catch2/         ← C++ unit tests (Catch2)
└── python/         ← Python unit tests for the generator

docs/               ← Sphinx documentation source
cmake/              ← CMake helpers, toolchains, find modules
```

---

## Model Files

Place your model files in any directory and pass it via `--model_dir`. Five file types are
recognised (JSON or YAML):

| File | Purpose |
|---|---|
| `groups.json` | Define groups — each group has a `baseId`, `version`, and persistence mode |
| `datapoints.json` | Define datapoints — type, access, default value, version, optional namespace |
| `structs.json` | Define custom struct types (nestable) |
| `enums.json` | Define enumerations — auto-ID or explicit values |
| `types.json` | Define alias types with optional compile-time range bounds |

### Example: groups.json

```json
{
  "Groups": [
    { "name": "DefaultGroup", "persistence": "None",   "baseId": "0x4000", "version": "1.0.1" },
    { "name": "CyclicGroup",  "persistence": "Cyclic", "baseId": "0x5000", "version": "2.0.0" }
  ]
}
```

### Example: datapoints.json

```json
{
  "Datapoints": [
    {
      "name": "test", "group": "DefaultGroup", "id": 1,
      "type": "int32_t", "default": 4211,
      "access": "READ_WRITE", "namespace": "Testify", "version": "1.0.1",
      "renamedFrom": [17]
    }
  ]
}
```

See the [Model File Reference](https://StephanKa.github.io/DataLayerGenerator/model.html) for the
full specification.

---

## Code Generator

```bash
python src/generator/generator.py \
  --model_dir   src/model \
  --out_dir     build/ \
  --template_dir src/template \
  --schema_dir   src/generator \
  --module_name  my_module      # optional pybind11 module name
```

Validate a model in CI without writing generated files:

```bash
python src/generator/generator.py \
  --model_dir src/model --out_dir build --template_dir src/template \
  --schema_dir src/generator --check
```

For a local browser UI to author canonical JSON models, install the generator requirements and run:

```bash
python src/generator/model_editor.py --model-dir src/model
```

Open `http://127.0.0.1:5000`. The editor supports the canonical `groups.json`, `datapoints.json`,
`structs.json`, `enums.json`, and `types.json` layout; use the generator directly for YAML or custom
split-file models.

Generated output under `build/generated/`:

| File | Content |
|---|---|
| `include/datalayer.h` | All group infos, types, enums, structs, `constinit` datapoint globals |
| `include/formatter.h` | `fmt::formatter<>` specialisations for every generated struct and enum |
| `datalayer_example/pythonBinding.cpp` | pybind11 module source |
| `doc/*.puml` | PlantUML class diagrams |
| `doc/overview.csv` | Datapoint overview table |

---

## Generated Python Bindings

Build the generated extension as a wheel and install it:

```powershell
uv venv
uv pip install --python .venv\Scripts\python.exe build
Push-Location datalayer_example
uv build
uv pip install --python ..\.venv\Scripts\python.exe --force-reinstall .\dist\datalayer_example-0.0.1-*.whl
Pop-Location
```

The module exposes generated struct classes plus static `get`/`set` methods for each datapoint.
With the sample model:

```python
import datalayer_example as dl

dl.Test.set(42)
print(dl.Test.get())

temperature = dl.Temperature(2350, 23.5)
temperature.value = 24.0
dl.Test4.set(temperature)

dl.Arraytest2.set([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
print(dl.Arraytest2.get())
```

The generated class names follow the datapoint names in the model; use `dir(dl)` to inspect an
extension generated from a custom model. Set `-DPYBIND11_MODULE_NAME=my_datalayer` when configuring
CMake to choose a different import name.

---

## CMake Options

| Option | Default | Description |
|---|---|---|
| `ENABLE_FMT` | `OFF` | Enable `fmt` library support; compiles with `-DUSE_FMT` |
| `ENABLE_FILE_PERSISTENCE` | `OFF` | Enable binary file persistence; compiles with `-DUSE_FILE_PERSISTENCE` |
| `ENABLE_PYBIND11` | `OFF` | Build pybind11 Python module |
| `PYBIND11_DISABLE_EXTRAS` | `OFF` | Disable pybind11 LTO/strip extras for faster development builds |
| `ENABLE_TESTING` | `ON` | Build Catch2 test suite |
| `ENABLE_DOCS` | `OFF` | Build Sphinx/Doxygen documentation |
| `DOCS_ONLY` | `OFF` | Skip all C++ targets and Conan — docs build only |

---

## Usage Example

```cpp
#include <include/datalayer.h>    // generated
#include <include/formatter.h>   // generated (requires ENABLE_FMT)
#include <fmt/format.h>

int main()
{
    // Print group structure
    DefaultGroup.printDatapoints();

    // Read / write a typed datapoint
    std::ignore = Testify::test.set(42);     // returns RangeCheck::ok
    int32_t val = Testify::test.get();       // val == 42

    // Range-checked alias type (min=100, max=200)
    auto rc = TestAlias.set(AliasTypeUint32t{42});
    // rc == DataLayer::Detail::RangeCheck::underflow

    // Struct datapoint
    constexpr Temperature a{ .raw = 1234, .value = 42.2f };
    std::ignore = test4.set(a);

    // Runtime-ID dispatch across all groups
    auto result = Dispatcher.setDatapoint(0x4001, 99);
    // result.success == true, result.check == RangeCheck::ok

#ifdef USE_FILE_PERSISTENCE
    // Persist and restore CyclicGroup
    CyclicGroup.serializeGroup("sample.bin");
    CyclicGroup.deserializeGroup("sample.bin");
#endif
}
```

### Transport and Change Notifications

Datapoints can notify application-owned code after a successful write. The callback receives the
new typed value and the supplied context pointer.

```cpp
void onTemperature(const Temperature& value, void* context) noexcept;
test4.setChangeCallback(onTemperature, applicationContext);
```

`mqttAdapter.h` and `canAdapter.h` deliberately do not include networking or driver libraries.
Implement `DataLayer::Mqtt::Client` or `DataLayer::Can::Bus`, then use the typed adapter to publish
values and apply validated incoming writes. CAN defaults to an 8-byte payload; instantiate
`DataLayer::Can::Adapter<decltype(Dispatcher), 64>` for CAN FD.

---

## Consuming as a Conan Package

```bash
conan create . --build=missing   # build & install into local cache
```

In your project's `conanfile.txt`:

```ini
[requires]
datalayer-generator/0.0.1

[options]
datalayer-generator/*:with_fmt=True

[generators]
CMakeDeps
CMakeToolchain
```

In your `CMakeLists.txt`:

```cmake
find_package(DataLayerGenerator REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE DataLayerGenerator::DataLayerGenerator)

generate_datalayer(my_app
    MODEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/model
    OUT_DIR   ${CMAKE_CURRENT_BINARY_DIR}
)
```

The `generate_datalayer()` helper is automatically available after `find_package` — it wires up the
Python generator as a stamp-file-based custom command so the generator only reruns when model files
change.

---

## Documentation

Full documentation — including API reference, model file specification, and integration guide — is
published automatically to GitHub Pages on every push to `main`:

**➡ https://StephanKa.github.io/DataLayerGenerator/**

To build the docs locally:

```bash
uv venv
uv pip install --python .venv/bin/python sphinx sphinx-rtd-theme breathe sphinx-copybutton
mkdir -p build-docs/doxygen
sed \
  -e "s|@DOXYGEN_INPUT_DIR@|$(pwd)/src/include|g" \
  -e "s|@DOXYGEN_OUTPUT_DIR@|$(pwd)/build-docs/doxygen|g" \
  docs/Doxyfile.in > build-docs/Doxyfile
doxygen build-docs/Doxyfile
sphinx-build -b html \
  -Dbreathe_projects.DataLayerGenerator="$(pwd)/build-docs/doxygen/xml" \
  docs/ build-docs/sphinx
# Output: build-docs/sphinx/index.html
```

---

## License

This project is released into the public domain under the [Unlicense](LICENSE).
