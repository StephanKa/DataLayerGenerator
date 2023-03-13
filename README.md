# Datalayer Generator

[![CMake](https://github.com/StephanKa/DataLayerGenerator/actions/workflows/build_cmake.yml/badge.svg?branch=main)](https://github.com/StephanKa/DataLayerGenerator/actions/workflows/build_cmake.yml)

## Description

This small project should present the possibility to use modern C++ for a datalayer. The datalayer could be used for MCU to read / write data in structures.
The advantages are:
- C++ 20 features
- compile time, as much as possible as constexpr and template meta programming
- version scheme in data points, e.g. to update data saved on EEPROM to update values
- default values can be used
- using of fmt library for printing data
  - generated struct formatter for fmt

### Generator

The code generator is based on python and Jinja2. The description files are needed to be placed under [**src/model**](src/model).
For writing model files you can choose json or yaml. For more information about the generation and capabilites see manual [here](src/generator/README.md)

### Formatter

The python generator will automatically generate [FMT](https://github.com/fmtlib/fmt) compatible formatter but only for structures.

### Docker

There are two different [docker images](docker). One image for CI and building and one for the developer itself.

## Example

The following example can be found in main.cpp.

```c++
#include "include/version.hpp"
#include <datalayer.h>
#include <fmt/format.h>
#include <formatter.h>


template<>
struct fmt::formatter<SoftwareVersion>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const SoftwareVersion &version, FormatContext &ctx)
    {
        return format_to(ctx.out(), "Major: {} Minor: {} Build: {} Githash: {}", version.Major, version.Minor, version.Patch, version.GitHash);
    }
};

int main()
{
    DefaultGroup.printDatapoints();
    CyclicGroup.printDatapoints();

    fmt::print("------------------------------------\nGroup: {:#06x}\n", DefaultGroupInfo.baseId);
    Testify::test.set(42);
    const auto versionTest = Testify::test.getVersion();
    fmt::print(R"(Datapoints
    id: {:#06x}
    value: {}
    version:
        Major = {}
        Minor = {}
        Build = {}
)",
      Testify::test.getId(),
      Testify::test(),
      versionTest.major,
      versionTest.minor,
      versionTest.build);

    constexpr Temperature a{ .raw = 1234, .value = 42.2F };
    test4.set(a);
    const auto test4Value = test4();
    const auto version4Test = test4.getVersion();
    fmt::print(R"(Datapoints
    id: {:#06x}
    value: {}
    version:
        Major = {}
        Minor = {}
        Build = {}
)",
      test4.getId(),
      test4Value,
      version4Test.major,
      version4Test.minor,
      version4Test.build);

    fmt::print("{}\n", SoftwareVersion{});
    fmt::print("\nPrint whole structure:\n");
    Dispatcher.printStructure();
    return 0;
}
```

The code above will output the following:

```c++
Group: DefaultGroup
test: 0x4001
est2: 0x4002
test3: 0x4003
Group: CyclicGroup
test4: 0x5004
------------------------------------
Group: 0x4000
Datapoints
    id: 0x4001
    value: 42
    version:
        Major = 1
        Minor = 0
        Build = 1
Datapoints
    id: 0x5004
    value:  raw = 1234 value = 42.2
    version:
        Major = 1
        Minor = 0
        Build = 1
Major: 0 Minor: 0 Build: 1 Githash: a84c11b

Print whole structure:
Structure:
Group: DefaultGroup
test: 0x4001
est2: 0x4002
test3: 0x4003
Group: CyclicGroup
test4: 0x5004
```

## Usage

For getting known preset have a look into [CMakePresets.json](CMakePresets.json).

Existing presets are for example:
- gcc-12-debug
- gcc-12-release
- clang-15-debug
- clang-15-release
- windows-2022-debug
- windows-2022-release
- win32-gcc-x64-mingw-debug
- win32-gcc-x64-mingw-release

```bash
cmake --preset <PRESET_NAME>
cmake --build --preset <PRESET_NAME>
ctest --preset <PRESET_NAME>
```

## To-Do's

- [ ] add coverage
- [ ] add serialization and deserialization
- [ ] add version handling
- [ ] version schema dynamisch
