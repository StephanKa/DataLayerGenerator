#include "include/version.hpp"
#include <datalayer.h>


template<> struct fmt::formatter<SoftwareVersion>
{
    template<typename ParseContext> constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template<typename FormatContext> auto format(const SoftwareVersion & /*version*/, FormatContext &ctx)
    {
        return format_to(ctx.out(), "Major: {} Minor: {} Build: {} Githash: {}", SoftwareVersion::Major, SoftwareVersion::Minor, SoftwareVersion::Patch, SoftwareVersion::GitHash);
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

    constexpr Temperature a{ 42.2f, 1234 };
    test4.set(a);
    const auto test4Value = test4();
    const auto version4Test = test4.getVersion();
    fmt::print(R"(Datapoints
    id: {:#06x}
    value: temp={} raw={}
    version:
        Major = {}
        Minor = {}
        Build = {}
)",
      test4.getId(),
      test4Value.value,
      test4Value.raw,
      version4Test.major,
      version4Test.minor,
      version4Test.build);

    fmt::print("{}", SoftwareVersion{});
    return 0;
}
