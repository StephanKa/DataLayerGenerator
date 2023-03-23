#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <datalayer.h>

namespace {
constexpr auto EPSILON = 0.1;

constexpr bool operator==(const Temperature &lhs, const Temperature &rhs) { return (lhs.raw == rhs.raw) && (static_cast<double>(std::fabs(lhs.value - rhs.value)) <= EPSILON); }
}// namespace

TEST_CASE("Test datapoint default values via get() method", "[Datapoints]")
{
    constexpr auto EXPECTED = 123.0;
    REQUIRE(test.get().raw == 4711);
    REQUIRE_THAT(test.get().value, Catch::Matchers::WithinRel(EXPECTED, EPSILON));
}

TEST_CASE("Test datapoint default values with () operator", "[Datapoints]")
{
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
    constexpr auto EXPECTED = 123.0;
    REQUIRE((initialValue == Temperature{ 4711, EXPECTED }));

    // write new data
    constexpr float newValue{ 123.4f };
    constexpr uint32_t newRaw{ 1234u };
    const Temperature newData{ newRaw, newValue };
    test.set(newData);

    // verify data
    const auto newlyWrittenValue = test.get();
    REQUIRE((newlyWrittenValue == Temperature{ newRaw, newValue }));
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
    Temperature value{};
    REQUIRE(DefaultGroup.getDatapoint(dpId, value));
    REQUIRE(value.raw == initialValue.raw);
    REQUIRE(test().raw == initialValue.raw);
    REQUIRE_THAT(test().value, Catch::Matchers::WithinRel(static_cast<double>(value.value), EPSILON));

    // write new data
    constexpr float newValue{ 321.4f };
    constexpr uint32_t newRaw{ 3214u };
    const Temperature newData{ newRaw, newValue };

    REQUIRE(DefaultGroup.setDatapoint(dpId, newData));
    REQUIRE((test() == Temperature{ newRaw, newValue }));
}

TEST_CASE("Test datapoint 'testWithoutDefaultValue' default values via get() method", "[Datapoints]") { REQUIRE((testWithoutDefaultValue.get() == Temperature{})); }

TEST_CASE("Test datapoint 'testWithoutDefaultValue' default values with () operator", "[Datapoints]") { REQUIRE((testWithoutDefaultValue() == Temperature{})); }

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

TEST_CASE("Test datapoint 'testWithoutDefaultValueWriteOnly' can't read", "[Datapoints]")
{
    constexpr auto dpId = testWithoutDefaultValueWriteOnly.getId();
    Temperature value{};
    REQUIRE(!DefaultGroup.getDatapoint(dpId, value));
}

TEST_CASE("Test datapoint 'testWithoutDefaultValue' set via group", "[Datapoints]")
{
    const auto initialValue = testWithoutDefaultValue.get();
    constexpr auto dpId = testWithoutDefaultValue.getId();
    Temperature value{};
    REQUIRE(DefaultGroup.getDatapoint(dpId, value));
    REQUIRE((value == initialValue));

    // write new data
    constexpr float newValue{ 321.4f };
    constexpr uint32_t newRaw{ 3214u };
    const Temperature newData{ newRaw, newValue };

    REQUIRE(!DefaultGroup.setDatapoint(dpId, newData));
}

TEST_CASE("Test datapoint 'testWithoutDefaultValue' access, write anyway", "[Datapoints]")
{
    constexpr Temperature testValue{ .raw = 444444, .value = 12345.0 };
    const auto val = testWithoutDefaultValue.get();
    static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValue.TypeAccess)>, Helper::READONLY>);
    REQUIRE(testWithoutDefaultValue.get().raw != testValue.raw);
    testWithoutDefaultValue = testValue;
    REQUIRE((testWithoutDefaultValue.get() == testValue));
    testWithoutDefaultValue = val;
}

TEST_CASE("Test datapoint 'testWithoutDefaultValueWriteOnly' access, read anyway", "[Datapoints]")
{
    static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValueWriteOnly.TypeAccess)>, Helper::WRITEONLY>);
    const auto val = testWithoutDefaultValueWriteOnly();
    REQUIRE((val == Temperature{}));
}

TEST_CASE("Test datapoint dispatcher get() existing", "[Datapoints]")
{
    Temperature temperatureTest{ .raw = 1234, .value = 11111.f };
    REQUIRE(Dispatcher.getDatapoint(testWithoutDefaultValue.getId(), temperatureTest));
    REQUIRE((testWithoutDefaultValue.get() == temperatureTest));
}

TEST_CASE("Test datapoint dispatcher get() not existing", "[Datapoints]")
{
    constexpr uint32_t dummyId = 42;
    Temperature temperatureTest{ .raw = 1234, .value = 11111.f };
    REQUIRE(!Dispatcher.getDatapoint(dummyId, temperatureTest));
}

TEST_CASE("Test datapoint dispatcher setDatapoint() not existing", "[Datapoints]")
{
    constexpr uint32_t dummyId = 42;
    constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.f };
    REQUIRE(!Dispatcher.setDatapoint(dummyId, temperatureTest));
}

TEST_CASE("Test datapoint dispatcher setDatapoint() existing with READONLY", "[Datapoints]")
{
    constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.f };
    REQUIRE(!Dispatcher.setDatapoint(testWithoutDefaultValue.getId(), temperatureTest));
}

TEST_CASE("Test datapoint dispatcher setDatapoint() existing with WRITEONLY", "[Datapoints]")
{
    const auto initialValue = testWithoutDefaultValueWriteOnly();
    constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.f };
    Temperature readValue{};
    REQUIRE(!Dispatcher.getDatapoint(testWithoutDefaultValueWriteOnly.getId(), readValue));
    REQUIRE(Dispatcher.setDatapoint(testWithoutDefaultValueWriteOnly.getId(), temperatureTest));
    REQUIRE((testWithoutDefaultValueWriteOnly() == temperatureTest));
    testWithoutDefaultValueWriteOnly = initialValue;
}

TEST_CASE("Test datapoint dispatcher setDatapoint() and getDatapoint() existing", "[Datapoints]")
{
    const auto initialValue = test();
    constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.f };
    Temperature readValue{};
    REQUIRE(Dispatcher.getDatapoint(test.getId(), readValue));
    REQUIRE(Dispatcher.setDatapoint(test.getId(), temperatureTest));

    REQUIRE((test() == temperatureTest));

    test = initialValue;
}
