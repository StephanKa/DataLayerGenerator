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
#include "datalayerBase.h"
#include "helper.h"
#include "include/version.hpp"
#include <datalayer.h>
#include <fmt/format.h>
#include <formatter.h>

using namespace std::string_view_literals;

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

template<>
struct fmt::formatter<Version>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const Version &version, FormatContext &ctx)
    {
        return format_to(ctx.out(), "Major: {} Minor: {} Build: {}", version.major, version.minor, version.build);
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
    constexpr auto version4Test = test4.getVersion();
    fmt::print("Test4 version: {}\n", version4Test);
    fmt::print(R"(Datapoints
    id: {:#06x}
    value: {}
)",
      test4.getId(),
      test4Value);

    fmt::print("{}\n", SoftwareVersion{});
    fmt::print("\nPrint whole structure:\n");
    Dispatcher.printStructure();

    fmt::print("arrayTest\n");
    for (const auto &value : arrayTest.get()) {
        fmt::print("{}\n", value);
    }

    fmt::print("arrayTest2\n");
    for (const auto &value : Testify::arrayTest2.get()) {
        fmt::print("{}\n", value);
    }
    fmt::print("errorCode: {}\n", errorCode());

#ifdef USE_FILE_PERSISTENCE
    const auto writeStatus = CyclicGroup.serializeGroup("sample.bin"sv);
    test4.set({ .raw = 1111, .value = 12.345f });
    const auto readStatus = CyclicGroup.deserializeGroup("sample.bin"sv);
    fmt::print("writeStatus: {}\nreadStatus: {}\n", writeStatus.size, readStatus.size);
    const bool result = test4().raw == a.raw;
    fmt::print("test4.raw: {}\n", test4().raw);
    return result && (writeStatus.size == readStatus.size) ? 0 : 1;
#else
    return 0;
#endif
}
```

The code above will output the following:

```c++
Group: DefaultGroup
test: 0x4001
est2: 0x4002
test3: 0x4003
arrayTest2: 0x400c
errorCode: 0x4018
errorCodeArray: 0x4019
structInStructType: 0x402a
Group: CyclicGroup
test4: 0x5004
arrayTest: 0x5005
------------------------------------
Group: 0x4000
Datapoints
    id: 0x4001
    value: 42
    version:
        Major = 1
        Minor = 0
        Build = 1
Test4 version: Major: 1 Minor: 0 Build: 1
Datapoints
    id: 0x5004
    value:  raw = 1234 value = 42.2
Major: 0 Minor: 0 Build: 1 Githash: c9ee00d

Print whole structure:
Structure:
Group: DefaultGroup
test: 0x4001
est2: 0x4002
test3: 0x4003
arrayTest2: 0x400c
errorCode: 0x4018
errorCodeArray: 0x4019
structInStructType: 0x402a
Group: CyclicGroup
test4: 0x5004
arrayTest: 0x5005
arrayTest
 raw = 5555 value = 123
 raw = 5555 value = 123
 raw = 5555 value = 123
 raw = 5555 value = 123
 raw = 5555 value = 123
 raw = 5555 value = 123
 raw = 5555 value = 123
 raw = 5555 value = 123
 raw = 5555 value = 123
 raw = 5555 value = 123
arrayTest2
1234
1234
1234
1234
1234
1234
1234
1234
1234
1234
errorCode: None
writeStatus: 124
readStatus: 124
test4.raw: 1234
```

## Usage

For getting known preset have a look into [CMakePresets.json](CMakePresets.json).

Existing presets are for example:
- clang-12-debug
- clang-12-release
- clang-13-debug
- clang-13-release
- clang-14-debug
- clang-14-release
- clang-15-debug
- clang-15-release
- clang-16-debug
- clang-16-release
- gcc-10-debug
- gcc-10-release
- gcc-11-debug
- gcc-11-release
- gcc-12-debug
- gcc-12-release
- windows-2019-debug
- windows-2019-release
- windows-2022-debug
- windows-2022-release
- win32-gcc-x64-mingw-debug
- win32-gcc-x64-mingw-release
- clang-15-debug-static-analysis
- gcc-arm-debug
- gcc-arm-release

```bash
cmake --preset <PRESET_NAME>
cmake --build --preset <PRESET_NAME>
ctest --preset <PRESET_NAME>
```

## Serialization

| data type       | Group version | Datapoint version | Container persistence description |
|-----------------|---------------|-------------------|-----------------------------------|
|                 | 12 Byte       | 12 Byte           |                                   |
| **std::string** |               |                   | 4 Byte (size) + dynamic bytes     |
| **std::array**  |               |                   | dynamic bytes                     |
| **structs**     |               |                   | dynamic bytes                     |


## To-Do's
- [ ] add coverage
- [ ] add a CMake option for developer to have a default version (e.g. 0.0.0)
- [ ] version schema dynamisch
- [ ] add splitting namespaces to separate files (amalgamation or single files)
