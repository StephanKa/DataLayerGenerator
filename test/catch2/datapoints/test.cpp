#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <datalayer.h>

namespace {
constexpr auto EPSILON = 0.1;

constexpr bool operator==(const Temperature &lhs, const Temperature &rhs)
{
    return (lhs.raw == rhs.raw) && (static_cast<double>(std::fabs(lhs.value - rhs.value)) <= EPSILON);
}
}// namespace

TEST_CASE("Test datapoints")
{
    const auto initalTestValue = test();
    const auto initalTestWithoutDefaultValueValue = testWithoutDefaultValue();
    const auto initalTestWithoutDefaultValueWriteOnlyValue = testWithoutDefaultValueWriteOnly();

    SECTION("datapoint default values via get() method")
    {
        constexpr auto EXPECTED = 123.0;
        REQUIRE(test.get().raw == 4711);
        REQUIRE_THAT(test.get().value, Catch::Matchers::WithinRel(EXPECTED, EPSILON));
    }

    SECTION("datapoint default values with () operator")
    {
        constexpr auto EXPECTED = 123.0;
        REQUIRE(test().raw == 4711);
        REQUIRE_THAT(test().value, Catch::Matchers::WithinRel(EXPECTED, EPSILON));
    }

    SECTION("version definition")
    {
        constexpr auto version = test.getVersion();
        REQUIRE(version.major == 1);
        REQUIRE(version.minor == 0);
        REQUIRE(version.build == 22);
    }

    SECTION("datapoint id definition")
    {
        constexpr auto expectedDefaultGroupID = 0x4000;
        constexpr uint16_t expectedDatapointId = 0x4;
        REQUIRE(DefaultGroupInfo.baseId == expectedDefaultGroupID);
        REQUIRE(test.getId() == (expectedDefaultGroupID + expectedDatapointId));
    }

    SECTION("datapoint type")
    {
        const auto value = test.get();
        constexpr Temperature expected{};
        static_assert(std::is_same_v<decltype(value), decltype(expected)>);
    }

    SECTION("datapoint access")
    {
        static_assert(std::is_same_v<std::remove_cv_t<decltype(test.TypeAccess)>, Helper::READWRITE>);
    }

    SECTION("datapoint read / write")
    {
        const auto initialValue = test.get();
        constexpr auto EXPECTED = 123.0;
        REQUIRE((initialValue == Temperature{ 4711, EXPECTED }));

        // write new data
        constexpr float newValue{ 123.4F };
        constexpr uint32_t newRaw{ 1234U };
        const Temperature newData{ newRaw, newValue };
        test.set(newData);

        // verify data
        const auto newlyWrittenValue = test.get();
        REQUIRE((newlyWrittenValue == Temperature{ newRaw, newValue }));
    }

    SECTION("datapoint name")
    {
        using namespace std::string_literals;
        REQUIRE(test.name == "test"s);
    }

    SECTION("datapoint set via group")
    {
        const auto initialValue = test.get();
        constexpr auto dpId = test.getId();
        Temperature value{};
        REQUIRE(DefaultGroup.getDatapoint(dpId, value));
        REQUIRE(value.raw == initialValue.raw);
        REQUIRE(test().raw == initialValue.raw);
        REQUIRE_THAT(test().value, Catch::Matchers::WithinRel(static_cast<double>(value.value), EPSILON));

        // write new data
        constexpr float newValue{ 321.4F };
        constexpr uint32_t newRaw{ 3214U };
        const Temperature newData{ newRaw, newValue };

        REQUIRE(DefaultGroup.setDatapoint(dpId, newData));
        REQUIRE((test() == Temperature{ newRaw, newValue }));
    }

    SECTION("datapoint 'testWithoutDefaultValue' default values via get() method")
    {
        REQUIRE((testWithoutDefaultValue.get() == Temperature{}));
    }

    SECTION("datapoint 'testWithoutDefaultValue' default values with () operator")
    {
        REQUIRE((testWithoutDefaultValue() == Temperature{}));
    }

    SECTION("'testWithoutDefaultValue' version definition")
    {
        constexpr auto version = testWithoutDefaultValue.getVersion();
        REQUIRE(version.major == 2);
        REQUIRE(version.minor == 10);
        REQUIRE(version.build == 223);
    }

    SECTION("datapoint 'testWithoutDefaultValue' access")
    {
        static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValue.TypeAccess)>, Helper::READONLY>);
    }

    SECTION("'testWithoutDefaultValueWriteOnly' version definition")
    {
        constexpr auto version = testWithoutDefaultValueWriteOnly.getVersion();
        REQUIRE(version.major == 0);
        REQUIRE(version.minor == 2);
        REQUIRE(version.build == 3);
    }

    SECTION("datapoint 'testWithoutDefaultValueWriteOnly' access")
    {
        static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValueWriteOnly.TypeAccess)>, Helper::WRITEONLY>);
    }

    SECTION("datapoint 'testWithoutDefaultValueWriteOnly' can't read")
    {
        constexpr auto dpId = testWithoutDefaultValueWriteOnly.getId();
        Temperature value{};
        REQUIRE(!DefaultGroup.getDatapoint(dpId, value));
    }

    SECTION("datapoint 'testWithoutDefaultValue' set via group")
    {
        const auto initialValue = testWithoutDefaultValue.get();
        constexpr auto dpId = testWithoutDefaultValue.getId();
        Temperature value{};
        REQUIRE(DefaultGroup.getDatapoint(dpId, value));
        REQUIRE((value == initialValue));

        // write new data
        constexpr float newValue{ 321.4F };
        constexpr uint32_t newRaw{ 3214U };
        const Temperature newData{ newRaw, newValue };

        REQUIRE(!DefaultGroup.setDatapoint(dpId, newData));
    }

    SECTION("datapoint 'testWithoutDefaultValue' access, write anyway")
    {
        constexpr Temperature testValue{ .raw = 444444, .value = 12345.0 };
        static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValue.TypeAccess)>, Helper::READONLY>);
        REQUIRE(testWithoutDefaultValue.get().raw != testValue.raw);
        testWithoutDefaultValue = testValue;
        REQUIRE((testWithoutDefaultValue.get() == testValue));
    }

    SECTION("datapoint 'testWithoutDefaultValueWriteOnly' access, read anyway")
    {
        static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValueWriteOnly.TypeAccess)>, Helper::WRITEONLY>);
        const auto val = testWithoutDefaultValueWriteOnly();
        REQUIRE((val == Temperature{}));
    }

    SECTION("datapoint dispatcher get() existing")
    {
        Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        REQUIRE(Dispatcher.getDatapoint(testWithoutDefaultValue.getId(), temperatureTest));
        REQUIRE((testWithoutDefaultValue.get() == temperatureTest));
    }

    SECTION("datapoint dispatcher get() not existing")
    {
        constexpr uint32_t dummyId = 42;
        Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        REQUIRE(!Dispatcher.getDatapoint(dummyId, temperatureTest));
    }

    SECTION("datapoint dispatcher setDatapoint() not existing")
    {
        constexpr uint32_t dummyId = 42;
        constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        REQUIRE(!Dispatcher.setDatapoint(dummyId, temperatureTest));
    }

    SECTION("datapoint dispatcher setDatapoint() existing with READONLY")
    {
        constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        REQUIRE(!Dispatcher.setDatapoint(testWithoutDefaultValue.getId(), temperatureTest));
    }

    SECTION("datapoint dispatcher setDatapoint() existing with WRITEONLY")
    {
        constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        Temperature readValue{};
        REQUIRE(!Dispatcher.getDatapoint(testWithoutDefaultValueWriteOnly.getId(), readValue));
        REQUIRE(Dispatcher.setDatapoint(testWithoutDefaultValueWriteOnly.getId(), temperatureTest));
        REQUIRE((testWithoutDefaultValueWriteOnly() == temperatureTest));
    }

    SECTION("datapoint dispatcher setDatapoint() and getDatapoint() existing")
    {
        constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        Temperature readValue{};
        REQUIRE(Dispatcher.getDatapoint(test.getId(), readValue));
        REQUIRE(Dispatcher.setDatapoint(test.getId(), temperatureTest));
        REQUIRE((test() == temperatureTest));
    }

    test = initalTestValue;
    testWithoutDefaultValue = initalTestWithoutDefaultValueValue;
    testWithoutDefaultValueWriteOnly = initalTestWithoutDefaultValueWriteOnlyValue;
}
