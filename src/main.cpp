#include "datalayerBase.h"// for DataPoint, GroupDataPointMapping, Dis...
#include "helper.h"// for Version
#include "include/version.hpp"// for SoftwareVersion
#include <datalayer.h>// for Temperature, test, test4, CyclicGroup
#include <fmt/format.h>// for print, basic_string_view, formatter
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
    // const auto version4Test = test4.getVersion();
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
    for (const auto &value : arrayTest.get()) { fmt::print("{}\n", value); }

    fmt::print("arrayTest2\n");
    for (const auto &value : Testify::arrayTest2.get()) { fmt::print("{}\n", value); }
    fmt::print("errorCode: {}\n", errorCode());
    return 0;
}
