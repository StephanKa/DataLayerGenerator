# DataLayerGenerator

[![CMake CI](https://github.com/StephanKa/DataLayerGenerator/actions/workflows/build_cmake.yml/badge.svg?branch=main)](https://github.com/StephanKa/DataLayerGenerator/actions/workflows/build_cmake.yml)
[![Documentation](https://github.com/StephanKa/DataLayerGenerator/actions/workflows/docs.yml/badge.svg?branch=main)](https://stephanKa.github.io/DataLayerGenerator/)

**📖 [Full documentation on GitHub Pages](https://StephanKa.github.io/DataLayerGenerator/)**

---

A **header-only C++20 framework** with a **Python/Jinja2 code generator** for building type-safe,
zero-allocation data layers — targeting embedded systems (MCU / bare-metal) as well as host
applications.

You describe your data model in JSON or YAML files. The generator produces a single `datalayer.h`
header that contains fully type-safe `constinit` datapoint globals, with access control enforced at
compile time, optional range checking, optional file persistence, optional `fmt` formatters, and
optional Python bindings.

---

## Features

| Feature | Description |
|---|---|
| **C++20** | Requires C++20 — leverages Concepts, NTTPs, `consteval`, `constinit`, `std::span` |
| **Header-only framework** | `src/include/` has no compiled artifacts and no external dependencies |
| **Compile-time access control** | `READ_ONLY`, `WRITE_ONLY`, `READ_WRITE` enforced via C++20 Concepts |
| **Range checking** | Alias types carry `Minimum`/`Maximum`; violations return `RangeCheck::underflow/overflow` |
| **Versioning** | Groups and individual datapoints carry `Version{major, minor, build}` for EEPROM upgrade paths |
| **Persistence** | Binary file serialization with group- and datapoint-level version validation (`-DENABLE_FILE_PERSISTENCE=ON`) |
| **fmt support** | Auto-generated `fmt::formatter<>` for all custom structs and enums (`-DENABLE_FMT=ON`) |
| **Python bindings** | Auto-generated `pybind11` module (`-DENABLE_PYBIND11=ON`) |
| **JSON & YAML models** | Write model files in JSON or YAML; mixed directories supported |
| **Cross-compilation** | ARM Cortex-M4 GCC toolchain included |
| **Conan package** | Distributable as a Conan `header-library` package with consumer helper CMake function |

---

## Quick Start

### Prerequisites

- CMake ≥ 3.19
- C++20 compiler (GCC 10+, Clang 15+, MSVC 2022)
- Python ≥ 3.9 with `pip install jinja2 jsonschema pyyaml`
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
      "access": "READ_WRITE", "namespace": "Testify", "version": "1.0.1"
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

Generated output under `build/generated/`:

| File | Content |
|---|---|
| `include/datalayer.h` | All group infos, types, enums, structs, `constinit` datapoint globals |
| `include/formatter.h` | `fmt::formatter<>` specialisations for every generated struct and enum |
| `datalayer_example/pythonBinding.cpp` | pybind11 module source |
| `doc/*.puml` | PlantUML class diagrams |
| `doc/overview.csv` | Datapoint overview table |

---

## CMake Options

| Option | Default | Description |
|---|---|---|
| `ENABLE_FMT` | `OFF` | Enable `fmt` library support; compiles with `-DUSE_FMT` |
| `ENABLE_FILE_PERSISTENCE` | `OFF` | Enable binary file persistence; compiles with `-DUSE_FILE_PERSISTENCE` |
| `ENABLE_PYBIND11` | `OFF` | Build pybind11 Python module |
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
pip install sphinx sphinx-rtd-theme breathe sphinx-copybutton
cmake -S . -B build-docs -DDOCS_ONLY=ON -DENABLE_DOCS=ON
cmake --build build-docs --target Sphinx
# Output: build-docs/docs/sphinx/index.html
```

---

## License

This project is released into the public domain under the [Unlicense](LICENSE).
