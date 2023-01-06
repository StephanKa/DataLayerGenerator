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
#include <formatter.h>


template<> struct fmt::formatter<SoftwareVersion>
{
    template<typename ParseContext> constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template<typename FormatContext> auto format(const SoftwareVersion &version, FormatContext &ctx)
    {
        return format_to(ctx.out(), "Major: {} Minor: {} Build: {} Githash: {}", version.Major, version.Minor, version.Patch, version.GitHash);
    }
};

int main()
{
    DefaultGroup.printDatapoints();
    CyclicGroup.printDatapoints();

    fmt::print("------------------------------------\nGroup: {}\n", DefaultGroupInfo.baseId);
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

    constexpr Temperature a{ .value = 42.2F, .raw = 1234 };
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

    fmt::print("{}", SoftwareVersion{});
    return 0;
}
```

The code above will output the following:

```c++
Group: DefaultGroup
test: 16385
est2: 16386
test3: 16387
Group: CyclicGroup
test4: 20484
------------------------------------
Group: 16384
Datapoints
    id: 0x4001
    value: 42
    version:
        Major = 1
        Minor = 0
        Build = 1
Datapoints
    id: 0x5004
    value:  value = 42.2 raw = 1234
    version:
        Major = 1
        Minor = 0
        Build = 1
Major: 0 Minor: 0 Build: 1 Githash: f36e9a4
```

## To-Do's

- [ ] add coverage
- [ ] add possibility to deactivate FMT
- [ ] add possibility to distinguish between MCU and desktop operating systems
- [ ] add serialization and deserialization
