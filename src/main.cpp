#include "datalayerBase.h"// for DataPoint, GroupDataPointMapping, Dis...
#include "helper.h"// for Version
#include "include/version.hpp"// for SoftwareVersion
#include <datalayer.h>// for Temperature, test, test4, CyclicGroup
#include <fmt/format.h>// for print, basic_string_view, formatter
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
