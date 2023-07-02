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
    const auto initialArrayTest2 = arrayTest2();

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
        constexpr Temperature newData{ newRaw, newValue };

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

    SECTION("datapoint serialization")
    {
        size_t index = 0;
        constexpr std::array expected = {
            std::byte{ 0x67 }, std::byte{ 0x12 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xf6 }, std::byte{ 0x42 }
        };
        const auto serializedDatapoint = test.serialize();

        REQUIRE(serializedDatapoint.size() == expected.size());
        for (const auto temp : serializedDatapoint) {
            REQUIRE(temp == expected.at(index));
            index++;
        }
    }

    SECTION("datapoint deserialization")
    {
        constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        const auto serializedDatapoint = test.serialize();
        std::array<std::byte, sizeof(Temperature)> initial{};
        std::copy(serializedDatapoint.begin(), serializedDatapoint.end(), initial.begin());

        test = temperatureTest;
        REQUIRE(test.get().raw == temperatureTest.raw);

        test.deserialize(initial);
        REQUIRE(test.get().raw == initalTestValue.raw);
    }

    SECTION("datapoint 'arrayTest' check correct array initialize")
    {
        constexpr Temperature temperatureTest{ .raw = 5555, .value = 123.0F };
        REQUIRE(arrayTest().size() == 10);
        for (const auto &temp : arrayTest()) {
            REQUIRE(temp.raw == temperatureTest.raw);
            REQUIRE(temp.value == temperatureTest.value);
        }
    }

    SECTION("datapoint 'arrayTest2' check correct array initialize")
    {
        const auto val = arrayTest2();
        REQUIRE(val.size() == 10);
        for (const auto &temp : val) { REQUIRE(temp == 1234); }
    }

    SECTION("datapoint 'arrayTest2' dispatcher get() existing")
    {
        using Return = std::remove_cvref_t<decltype(arrayTest2.get())>;
        Return temperatureTest;

        REQUIRE(Dispatcher.getDatapoint(arrayTest2.getId(), temperatureTest));
        REQUIRE((arrayTest2.get() == temperatureTest));
        REQUIRE(arrayTest2.getId() == 7 + DefaultGroupInfo.baseId);
    }

    SECTION("datapoint 'arrayTest' dispatcher get() existing")
    {
        constexpr Temperature expectedTemperature{ .raw = 5555, .value = 123.0F };
        using Return = std::remove_cvref_t<decltype(arrayTest.get())>;
        Return temperatureTest;

        REQUIRE(Dispatcher.getDatapoint(arrayTest.getId(), temperatureTest));
        REQUIRE((arrayTest.get().size() == temperatureTest.size()));
        for (const auto &temp : temperatureTest) {
            REQUIRE(temp.raw == expectedTemperature.raw);
            REQUIRE_THAT(temp.value, Catch::Matchers::WithinRel(static_cast<double>(expectedTemperature.value), EPSILON));
        }
        REQUIRE(arrayTest.getId() == 5 + DefaultGroupInfo.baseId);
    }

    SECTION("datapoint 'arrayTest2' dispatcher get() existing")
    {
        using Return = std::remove_cvref_t<decltype(arrayTest2.get())>;
        Return valueTest;
        int32_t index = 0;
        for (auto &temp : valueTest) {
            temp = index;
            ++index;
        }

        REQUIRE(Dispatcher.setDatapoint(arrayTest2.getId(), valueTest));
        REQUIRE(valueTest == arrayTest2.get());
        REQUIRE(arrayTest2.size() == 10);
    }

    SECTION("datapoint 'arrayTest2' get() by index")
    {
        using Return = std::remove_cvref_t<decltype(arrayTest2.get().at(0))>;
        Return valueTest = arrayTest2.get(1);

        REQUIRE(valueTest == 1234);
    }

    SECTION("datapoint 'arrayTest' get() by index")
    {
        constexpr Temperature expectedTemperature{ .raw = 5555, .value = 123.0F };
        using Return = std::remove_cvref_t<decltype(arrayTest.get().at(0))>;
        Return valueTest = arrayTest.get(1);


        REQUIRE(valueTest.raw == expectedTemperature.raw);
        REQUIRE_THAT(valueTest.value, Catch::Matchers::WithinRel(static_cast<double>(expectedTemperature.value), EPSILON));
    }

    SECTION("datapoint 'arrayTest2' set() by index")
    {
        const auto expected = arrayTest2.get();
        constexpr size_t changedIndex = 5;
        constexpr int32_t expectedValue = 42;

        arrayTest2.set(changedIndex, expectedValue);
        REQUIRE(arrayTest2.get(changedIndex) != expected.at(changedIndex));
        REQUIRE(arrayTest2.get(changedIndex) == expectedValue);
    }

    SECTION("datapoint array serialization")
    {
        size_t index = 0;
        const auto serializedDatapoint = arrayTest2.serialize();
        constexpr std::array expected = {
            // clang-format off
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 },
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 },
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 },
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 },
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }
            // clang-format on
        };

        REQUIRE(serializedDatapoint.size() == expected.size());
        for (const auto temp : serializedDatapoint) {
            REQUIRE(temp == expected.at(index));
            index++;
        }
    }

    SECTION("datapoint array deserialization")
    {
        using Return = std::remove_cvref_t<decltype(arrayTest2.get())>;
        constexpr Return valueTest{};
        constexpr std::array expected = {
            // clang-format off
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 },
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 },
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 },
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 },
            std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }, std::byte{ 0xD2 }, std::byte{ 0x04 }, std::byte{ 0x0 }, std::byte{ 0x0 }
            // clang-format on
        };

        arrayTest2.set(valueTest);
        REQUIRE(arrayTest2.get() == valueTest);
        arrayTest2.deserialize(expected);
        REQUIRE(arrayTest2.get() == initialArrayTest2);
    }

    test = initalTestValue;
    testWithoutDefaultValue = initalTestWithoutDefaultValueValue;
    testWithoutDefaultValueWriteOnly = initalTestWithoutDefaultValueWriteOnlyValue;
    arrayTest2 = initialArrayTest2;
}
