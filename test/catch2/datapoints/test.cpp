#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <datalayer.h>

TEST_CASE("Test datapoint default values via get() method", "[Datapoints]")
{
    constexpr auto EPSILON = 0.1;
    constexpr auto EXPECTED = 123.0;
    REQUIRE(test.get().raw == 4711);
    REQUIRE_THAT(test.get().value, Catch::Matchers::WithinRel(EXPECTED, EPSILON));
}

TEST_CASE("Test datapoint default values with () operator", "[Datapoints]")
{
    constexpr auto EPSILON = 0.1;
    constexpr auto EXPECTED = 123.0;
    REQUIRE(test().raw == 4711);
    REQUIRE_THAT(test().value, Catch::Matchers::WithinRel(EXPECTED, EPSILON));
}

TEST_CASE("Test version definition", "[Datapoints]")
{
    constexpr auto version = test.getVersion();
    REQUIRE(version.major == 1);
    REQUIRE(version.minor == 0);
    REQUIRE(version.build == 22);
}

TEST_CASE("Test datapoint id definition", "[Datapoints]")
{
    constexpr auto expectedDefaultGroupID = 0x4000;
    constexpr uint16_t expectedDatapointId = 0x4;
    REQUIRE(DefaultGroupInfo.baseId == expectedDefaultGroupID);
    REQUIRE(test.getId() == (expectedDefaultGroupID + expectedDatapointId));
}

TEST_CASE("Test datapoint type", "[Datapoints]")
{
    const auto value = test.get();
    constexpr Temperature expected{};
    static_assert(std::is_same_v<decltype(value), decltype(expected)>);
}

TEST_CASE("Test datapoint access", "[Datapoints]") { static_assert(std::is_same_v<std::remove_cv_t<decltype(test.TypeAccess)>, Helper::READWRITE>); }

TEST_CASE("Test datapoint read / write", "[Datapoints]")
{
    const auto initialValue = test.get();
    constexpr auto EPSILON = 0.1;
    constexpr auto EXPECTED = 123.0;
    REQUIRE(initialValue.raw == 4711);
    REQUIRE_THAT(test.get().value, Catch::Matchers::WithinRel(EXPECTED, EPSILON));

    // write new data
    constexpr float newValue{ 123.4f };
    constexpr uint32_t newRaw{ 1234u };
    const Temperature newData{ newValue, newRaw };
    test.set(newData);

    // verify data
    const auto newlyWrittenValue = test.get();
    REQUIRE(newlyWrittenValue.raw == newRaw);
    REQUIRE_THAT(newlyWrittenValue.value, Catch::Matchers::WithinRel(static_cast<double>(newValue), EPSILON));
}

TEST_CASE("Test datapoint name", "[Datapoints]")
{
    using namespace std::string_literals;
    REQUIRE(test.name == "test"s);
}

TEST_CASE("Test datapoint set via group", "[Datapoints]")
{
    const auto initialValue = test.get();
    constexpr auto dpId = test.getId();
    constexpr auto EPSILON = 0.1;
    Temperature value{};
    REQUIRE(DefaultGroup.getDatapoint(dpId, value));
    REQUIRE(value.raw == initialValue.raw);
    REQUIRE(test().raw == initialValue.raw);
    REQUIRE_THAT(test().value, Catch::Matchers::WithinRel(static_cast<double>(value.value), EPSILON));

    // write new data
    constexpr float newValue{ 321.4f };
    constexpr uint32_t newRaw{ 3214u };
    const Temperature newData{ newValue, newRaw };

    REQUIRE(DefaultGroup.setDatapoint(dpId, newData));
    REQUIRE(test().raw == newRaw);
    REQUIRE_THAT(test().value, Catch::Matchers::WithinRel(static_cast<double>(newValue), EPSILON));
}

TEST_CASE("Test datapoint 'testWithoutDefaultValue' default values via get() method", "[Datapoints]")
{
    constexpr auto EPSILON = 0.1;
    constexpr auto EXPECTED = 0.0;
    REQUIRE(testWithoutDefaultValue.get().raw == 0);
    REQUIRE_THAT(testWithoutDefaultValue.get().value, Catch::Matchers::WithinRel(EXPECTED, EPSILON));
}

TEST_CASE("Test datapoint 'testWithoutDefaultValue' default values with () operator", "[Datapoints]")
{
    constexpr auto EPSILON = 0.1;
    constexpr auto EXPECTED = 0.0;
    REQUIRE(testWithoutDefaultValue().raw == 0);
    REQUIRE_THAT(testWithoutDefaultValue().value, Catch::Matchers::WithinRel(EXPECTED, EPSILON));
}

TEST_CASE("Test 'testWithoutDefaultValue' version definition", "[Datapoints]")
{
    constexpr auto version = testWithoutDefaultValue.getVersion();
    REQUIRE(version.major == 2);
    REQUIRE(version.minor == 10);
    REQUIRE(version.build == 223);
}

TEST_CASE("Test datapoint 'testWithoutDefaultValue' access", "[Datapoints]")
{
    static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValue.TypeAccess)>, Helper::READONLY>);
}

TEST_CASE("Test 'testWithoutDefaultValueWriteOnly' version definition", "[Datapoints]")
{
    constexpr auto version = testWithoutDefaultValueWriteOnly.getVersion();
    REQUIRE(version.major == 0);
    REQUIRE(version.minor == 2);
    REQUIRE(version.build == 3);
}

TEST_CASE("Test datapoint 'testWithoutDefaultValueWriteOnly' access", "[Datapoints]")
{
    static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValueWriteOnly.TypeAccess)>, Helper::WRITEONLY>);
}
