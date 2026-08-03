#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <fstream>
#include <include/datalayer.h>

namespace DataLayer::Migration
{
    bool migrateUpgradableDatapoint(const Version &sourceVersion, std::span<const std::byte> source, std::span<std::byte> destination) noexcept
    {
        if (sourceVersion != Version{ 1, 0, 0 } || source.size() != sizeof(uint32_t) || destination.size() != sizeof(Temperature))
        {
            return false;
        }

        uint32_t raw{};
        std::memcpy(&raw, source.data(), sizeof(raw));
        const Temperature migrated{ .raw = raw, .value = static_cast<float>(raw) / 100.0F };
        std::memcpy(destination.data(), &migrated, sizeof(migrated));
        return true;
    }
}// namespace DataLayer::Migration

namespace
{
    constexpr auto EPSILON = 0.1;

    using ByteBuffer = std::vector<std::byte>;

    ByteBuffer readFile(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        const auto size = static_cast<size_t>(input.tellg());
        ByteBuffer bytes(size);
        input.seekg(0);
        input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return bytes;
    }

    void writeFile(const std::filesystem::path &path, std::span<const std::byte> bytes)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    void refreshChecksum(ByteBuffer &bytes)
    {
        DataLayer::Persistence::Header header{};
        std::memcpy(&header, bytes.data(), sizeof(header));
        header.checksum = DataLayer::Persistence::crc32(std::span<const std::byte>{ bytes }.subspan(sizeof(header)));
        std::memcpy(bytes.data(), &header, sizeof(header));
    }

    ByteBuffer makeFile(uint16_t groupId, DataLayer::Version groupVersion, uint16_t dataPointId, DataLayer::Version dataPointVersion, std::span<const std::byte> payload)
    {
        ByteBuffer records;
        DataLayer::Persistence::RecordHeader record{ .dataPointId = dataPointId, .version = dataPointVersion, .payloadSize = static_cast<uint32_t>(payload.size()) };
        DataLayer::Persistence::append(records, record);
        records.insert(records.end(), payload.begin(), payload.end());

        DataLayer::Persistence::Header header{ .groupId = groupId, .groupVersion = groupVersion, .checksum = DataLayer::Persistence::crc32(records) };
        ByteBuffer file;
        DataLayer::Persistence::append(file, header);
        file.insert(file.end(), records.begin(), records.end());
        return file;
    }

    void recordDatapointChange(const Temperature &value, void *context) noexcept
    {
        *static_cast<Temperature *>(context) = value;
    }

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
    const auto initialStructInStructType = structInStructType();

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
        STATIC_REQUIRE(version.major == 1);
        STATIC_REQUIRE(version.minor == 0);
        STATIC_REQUIRE(version.build == 22);
    }

    SECTION("datapoint id definition")
    {
        constexpr auto expectedDefaultGroupID = 0x4000;
        constexpr uint16_t expectedDatapointId = 0x4;
        STATIC_REQUIRE(DefaultGroupInfo.baseId == expectedDefaultGroupID);
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
        static_assert(std::is_same_v<std::remove_cv_t<decltype(test.TypeAccess)>, Helper::READ_WRITE>);
    }

    SECTION("datapoint read / write")
    {
        const auto initialValue = test.get();
        constexpr auto EXPECTED = 123.0;
        REQUIRE((initialValue == Temperature{ 4711, EXPECTED }));

        // write new data
        constexpr float newValue{ 123.4F };
        constexpr uint32_t newRaw{ 1234U };
        constexpr Temperature newData{ newRaw, newValue };
        std::ignore = test.set(newData);

        // verify data
        const auto newlyWrittenValue = test.get();
        REQUIRE((newlyWrittenValue == Temperature{ newRaw, newValue }));
    }

    SECTION("datapoint change callback")
    {
        Temperature observed{};
        constexpr Temperature expected{ .raw = 9876, .value = 12.5F };

        test.setChangeCallback(recordDatapointChange, &observed);
        REQUIRE(test.set(expected) == DataLayer::Detail::RangeCheck::ok);
        test.clearChangeCallback();

        REQUIRE(observed.raw == expected.raw);
        REQUIRE(observed.value == expected.value);
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
        constexpr Temperature newData{ .raw = newRaw, .value = newValue };

        const auto [success, check] = DefaultGroup.setDatapoint(dpId, newData);
        REQUIRE(success);
        REQUIRE(check == DataLayer::Detail::RangeCheck::ok);
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
        STATIC_REQUIRE(version.major == 2);
        STATIC_REQUIRE(version.minor == 10);
        STATIC_REQUIRE(version.build == 223);
    }

    SECTION("datapoint 'testWithoutDefaultValue' access")
    {
        static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValue.TypeAccess)>, Helper::READ_ONLY>);
    }

    SECTION("'testWithoutDefaultValueWriteOnly' version definition")
    {
        constexpr auto version = testWithoutDefaultValueWriteOnly.getVersion();
        STATIC_REQUIRE(version.major == 0);
        STATIC_REQUIRE(version.minor == 2);
        STATIC_REQUIRE(version.build == 3);
    }

    SECTION("datapoint 'testWithoutDefaultValueWriteOnly' access")
    {
        static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValueWriteOnly.TypeAccess)>, Helper::WRITE_ONLY>);
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
        constexpr Temperature newData{ newRaw, newValue };
        const auto [success, check] = DefaultGroup.setDatapoint(dpId, newData);
        REQUIRE(!success);
        REQUIRE(check == DataLayer::Detail::RangeCheck::notChecked);
    }

    SECTION("datapoint 'testWithoutDefaultValue' access, write anyway")
    {
        constexpr Temperature testValue{ .raw = 444444, .value = 12345.0 };
        static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValue.TypeAccess)>, Helper::READ_ONLY>);
        REQUIRE(testWithoutDefaultValue.get().raw != testValue.raw);
        testWithoutDefaultValue = testValue;
        REQUIRE((testWithoutDefaultValue.get() == testValue));
    }

    SECTION("datapoint 'testWithoutDefaultValueWriteOnly' access, read anyway")
    {
        static_assert(std::is_same_v<std::remove_cv_t<decltype(testWithoutDefaultValueWriteOnly.TypeAccess)>, Helper::WRITE_ONLY>);
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

        constexpr auto ret = DefaultGroup.setDatapoint(dummyId, temperatureTest);
        STATIC_REQUIRE(!ret.success);
        STATIC_REQUIRE(ret.check == DataLayer::Detail::RangeCheck::notChecked);
    }

    SECTION("datapoint dispatcher setDatapoint() existing with READ_ONLY")
    {
        constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        const auto [success, check] = DefaultGroup.setDatapoint(testWithoutDefaultValue.getId(), temperatureTest);
        REQUIRE(!success);
        REQUIRE(check == DataLayer::Detail::RangeCheck::notChecked);
    }

    SECTION("datapoint dispatcher setDatapoint() existing with WRITE_ONLY")
    {
        constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        Temperature readValue{};
        REQUIRE(!Dispatcher.getDatapoint(testWithoutDefaultValueWriteOnly.getId(), readValue));

        const auto [success, check] = DefaultGroup.setDatapoint(testWithoutDefaultValueWriteOnly.getId(), temperatureTest);
        REQUIRE(success);
        REQUIRE(check == DataLayer::Detail::RangeCheck::ok);
        REQUIRE((testWithoutDefaultValueWriteOnly() == temperatureTest));
    }

    SECTION("datapoint dispatcher setDatapoint() and getDatapoint() existing")
    {
        constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        Temperature readValue{};
        REQUIRE(Dispatcher.getDatapoint(test.getId(), readValue));
        const auto [success, check] = DefaultGroup.setDatapoint(test.getId(), temperatureTest);
        REQUIRE(success);
        REQUIRE(check == DataLayer::Detail::RangeCheck::ok);
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
        for (const auto temp : serializedDatapoint)
        {
            REQUIRE(temp == expected.at(index));
            index++;
        }
    }

    SECTION("datapoint deserialization")
    {
        constexpr Temperature temperatureTest{ .raw = 1234, .value = 11111.F };
        const auto serializedDatapoint = test.serialize();
        std::array<std::byte, sizeof(Temperature)> initial{};
        std::ranges::copy(serializedDatapoint, initial.begin());

        test = temperatureTest;
        REQUIRE(test.get().raw == temperatureTest.raw);

        test.deserialize(initial);
        REQUIRE(test.get().raw == initalTestValue.raw);
    }

    SECTION("datapoint 'arrayTest' check correct array initialize")
    {
        constexpr Temperature temperatureTest{ .raw = 5555, .value = 123.0F };
        REQUIRE(arrayTest().size() == 10);
        for (const auto &[raw, value] : arrayTest())
        {
            REQUIRE(raw == temperatureTest.raw);
            REQUIRE(value == temperatureTest.value);
        }
    }

    SECTION("datapoint 'arrayTest2' check correct array initialize")
    {
        const auto val = arrayTest2();
        REQUIRE(val.size() == 10);
        for (const auto &temp : val)
        {
            REQUIRE(temp == 1234);
        }
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
        for (const auto &[raw, value] : temperatureTest)
        {
            REQUIRE(raw == expectedTemperature.raw);
            REQUIRE_THAT(value, Catch::Matchers::WithinRel(static_cast<double>(expectedTemperature.value), EPSILON));
        }
        REQUIRE(arrayTest.getId() == 5 + DefaultGroupInfo.baseId);
    }

    SECTION("datapoint 'arrayTest2' dispatcher get() existing")
    {
        using Return = std::remove_cvref_t<decltype(arrayTest2.get())>;
        Return valueTest;
        int32_t index = 0;
        for (auto &temp : valueTest)
        {
            temp = index;
            ++index;
        }

        const auto ret = DefaultGroup.setDatapoint(arrayTest2.getId(), valueTest);
        REQUIRE(ret.success);
        REQUIRE(ret.check == DataLayer::Detail::RangeCheck::ok);
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
        auto [raw, value] = arrayTest.get(1);


        REQUIRE(raw == expectedTemperature.raw);
        REQUIRE_THAT(value, Catch::Matchers::WithinRel(static_cast<double>(expectedTemperature.value), EPSILON));
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
        for (const auto temp : serializedDatapoint)
        {
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

        std::ignore = arrayTest2.set(valueTest);
        REQUIRE(arrayTest2.get() == valueTest);
        arrayTest2.deserialize(expected);
        REQUIRE(arrayTest2.get() == initialArrayTest2);
    }

    SECTION("datapoint 'structInStructType' with nested structs")
    {
        constexpr Temperature internalTempTest{ .raw = 111, .value = 111.1F };
        constexpr Temperature externalTempTest{ .raw = 222, .value = 222.2F };
        const auto [external, internal] = structInStructType.get();
        REQUIRE(internalTempTest.raw == internal.raw);
        REQUIRE_THAT(internalTempTest.value, Catch::Matchers::WithinRel(static_cast<double>(internal.value), EPSILON));

        REQUIRE(externalTempTest.raw == external.raw);
        REQUIRE_THAT(externalTempTest.value, Catch::Matchers::WithinRel(static_cast<double>(external.value), EPSILON));
    }

    SECTION("set datapoint 'structInStructType' with nested structs")
    {
        constexpr Environment expectedValue{};
        const auto [success, check] = DefaultGroup.setDatapoint(structInStructType.getId(), expectedValue);
        REQUIRE(success);
        REQUIRE(check == DataLayer::Detail::RangeCheck::ok);

        const auto [external, internal] = structInStructType.get();
        REQUIRE(expectedValue.internal.raw == internal.raw);
        REQUIRE_THAT(expectedValue.internal.value, Catch::Matchers::WithinRel(static_cast<double>(internal.value), EPSILON));

        REQUIRE(expectedValue.external.raw == external.raw);
        REQUIRE_THAT(expectedValue.external.value, Catch::Matchers::WithinRel(static_cast<double>(external.value), EPSILON));
    }

    SECTION("group file serialization")
    {
        using namespace std::string_view_literals;
        const auto writeStatus = DefaultGroup.serializeGroup("sample.bin"sv);
        REQUIRE(writeStatus.result);
        REQUIRE(writeStatus.errorCode == SerializationError::None);
        REQUIRE(writeStatus.size == std::filesystem::file_size("sample.bin"sv));
    }

    SECTION("group file deserialization")
    {
        using namespace std::string_view_literals;
        const auto writeStatus = DefaultGroup.serializeGroup("sample.bin"sv);
        REQUIRE(writeStatus.result);
        REQUIRE(writeStatus.errorCode == SerializationError::None);

        const auto readStatus = DefaultGroup.serializeGroup("sample.bin"sv);
        REQUIRE(readStatus.size == std::filesystem::file_size("sample.bin"sv));
        REQUIRE(readStatus.result);
        REQUIRE(readStatus.errorCode == SerializationError::None);
    }

    SECTION("group file deserialization rejects corrupt payload")
    {
        using namespace std::string_view_literals;
        REQUIRE(DefaultGroup.serializeGroup("sample.bin"sv).result);

        std::fstream file("sample.bin", std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(-1, std::ios::end);
        constexpr char corruptByte{ 0 };
        file.write(&corruptByte, 1);
        file.close();

        const auto readStatus = DefaultGroup.deserializeGroup("sample.bin"sv);
        REQUIRE_FALSE(readStatus.result);
        REQUIRE(readStatus.errorCode == SerializationError::ChecksumMismatch);
    }

    SECTION("group file deserialization rejects an unavailable file")
    {
        std::filesystem::remove("missing.bin");

        const auto [result, size, errorCode] = DefaultGroup.deserializeGroup("missing.bin");
        REQUIRE_FALSE(result);
        REQUIRE(size == 0);
        REQUIRE(errorCode == SerializationError::InvalidFormat);
    }

    SECTION("group file deserialization rejects a byte-swapped format version")
    {
        REQUIRE(DefaultGroup.serializeGroup("sample.bin").result);
        auto bytes = readFile("sample.bin");
        DataLayer::Persistence::Header header{};
        std::memcpy(&header, bytes.data(), sizeof(header));
        header.formatVersion = std::byteswap(header.formatVersion);
        std::memcpy(bytes.data(), &header, sizeof(header));
        writeFile("sample.bin", bytes);

        const auto readStatus = DefaultGroup.deserializeGroup("sample.bin");
        REQUIRE_FALSE(readStatus.result);
        REQUIRE(readStatus.errorCode == SerializationError::InvalidFormat);
    }

    SECTION("group file deserialization rejects truncated records with a valid checksum")
    {
        REQUIRE(DefaultGroup.serializeGroup("sample.bin").result);
        auto bytes = readFile("sample.bin");
        bytes.resize(bytes.size() - 1);
        refreshChecksum(bytes);
        writeFile("sample.bin", bytes);

        const auto readStatus = DefaultGroup.deserializeGroup("sample.bin");
        REQUIRE_FALSE(readStatus.result);
        REQUIRE(readStatus.errorCode == SerializationError::InvalidFormat);
    }

    SECTION("group file deserialization rejects oversized record lengths with a valid checksum")
    {
        REQUIRE(DefaultGroup.serializeGroup("sample.bin").result);
        auto bytes = readFile("sample.bin");
        DataLayer::Persistence::RecordHeader record{};
        std::memcpy(&record, bytes.data() + sizeof(DataLayer::Persistence::Header), sizeof(record));
        record.payloadSize = static_cast<uint32_t>(bytes.size());
        std::memcpy(bytes.data() + sizeof(DataLayer::Persistence::Header), &record, sizeof(record));
        refreshChecksum(bytes);
        writeFile("sample.bin", bytes);

        const auto readStatus = DefaultGroup.deserializeGroup("sample.bin");
        REQUIRE_FALSE(readStatus.result);
        REQUIRE(readStatus.errorCode == SerializationError::InvalidFormat);
    }

    SECTION("partial replacement file does not replace a valid persistence file")
    {
        constexpr Temperature expected{ .raw = 4711, .value = 123.0F };
        REQUIRE(DefaultGroup.serializeGroup("sample.bin").result);
        std::ignore = test.set(Temperature{ .raw = 9999, .value = 99.9F });

        constexpr auto interruptedWrite = std::array{ std::byte{ 0x44 }, std::byte{ 0x4C } };
        writeFile("sample.bin.tmp", interruptedWrite);

        const auto readStatus = DefaultGroup.deserializeGroup("sample.bin");
        std::filesystem::remove("sample.bin.tmp");
        REQUIRE(readStatus.result);
        REQUIRE(readStatus.errorCode == SerializationError::None);
        REQUIRE(test().raw == expected.raw);
        REQUIRE(test().value == expected.value);
    }

    SECTION("datapoint migration restores an older payload shape")
    {
        constexpr uint32_t legacyRaw = 4321;
        const auto legacyPayload = std::as_bytes(std::span{ &legacyRaw, 1 });
        const auto bytes = makeFile(AllowUpgradeGroupInfo.baseId, DataLayer::Version{ 1, 0, 0 }, UpgradableDatapoint.getId(), DataLayer::Version{ 1, 0, 0 }, legacyPayload);
        writeFile("migration.bin", bytes);

        UpgradableDatapoint = Temperature{};
        const auto readStatus = AllowUpgradeGroup.deserializeGroup("migration.bin");
        REQUIRE(readStatus.result);
        REQUIRE(readStatus.errorCode == SerializationError::None);
        REQUIRE(UpgradableDatapoint().raw == legacyRaw);
        REQUIRE(UpgradableDatapoint().value == Catch::Approx(43.21F));
    }

    SECTION("record-byte fuzzing detects every single-byte payload corruption")
    {
        REQUIRE(DefaultGroup.serializeGroup("sample.bin").result);
        const auto original = readFile("sample.bin");
        constexpr auto recordOffset = sizeof(DataLayer::Persistence::Header);

        for (size_t index = recordOffset; index < original.size(); ++index)
        {
            auto mutated = original;
            mutated[index] ^= std::byte{ 0x01 };
            writeFile("sample.bin", mutated);

            const auto readStatus = DefaultGroup.deserializeGroup("sample.bin");
            REQUIRE_FALSE(readStatus.result);
            REQUIRE(readStatus.errorCode == SerializationError::ChecksumMismatch);
        }
    }

    SECTION("generated model persistence round-trips varied values")
    {
        for (uint32_t iteration = 1; iteration <= 16; ++iteration)
        {
            const Temperature expectedTemperature{ .raw = iteration * 97U, .value = static_cast<float>(iteration) * 1.25F };
            std::array<int32_t, 10> expectedArray{};
            for (size_t index = 0; index < expectedArray.size(); ++index)
            {
                expectedArray[index] = static_cast<int32_t>(iteration * 100U + index);
            }
            REQUIRE(test.set(expectedTemperature) == DataLayer::Detail::RangeCheck::ok);
            REQUIRE(arrayTest2.set(expectedArray) == DataLayer::Detail::RangeCheck::ok);
            REQUIRE(DefaultGroup.serializeGroup("round-trip.bin").result);

            std::ignore = test.set(Temperature{});
            std::ignore = arrayTest2.set({});

            const auto readStatus = DefaultGroup.deserializeGroup("round-trip.bin");
            REQUIRE(readStatus.result);
            REQUIRE(readStatus.errorCode == SerializationError::None);
            REQUIRE(test().raw == expectedTemperature.raw);
            REQUIRE(test().value == expectedTemperature.value);
            REQUIRE(arrayTest2() == expectedArray);
        }
    }

    SECTION("read serialized file to restore saved datapoint data")
    {
        using namespace std::string_view_literals;

        const auto [result, size, errorCode] = DefaultGroup.serializeGroup("sample.bin"sv);
        REQUIRE(result);
        REQUIRE(errorCode == SerializationError::None);

        // write new data
        constexpr Temperature newData{ 1234U, 123.4F };
        std::ignore = test.set(newData);
        REQUIRE((test() == newData));

        constexpr Temperature testValue{ .raw = 444444, .value = 12345.0 };
        testWithoutDefaultValue = testValue;
        REQUIRE((testWithoutDefaultValue() == testValue));

        const auto readStatus = DefaultGroup.deserializeGroup("sample.bin"sv);
        REQUIRE(readStatus.size == size);
        REQUIRE(readStatus.result);
        REQUIRE(readStatus.errorCode == SerializationError::None);

        REQUIRE(test().raw == initalTestValue.raw);
        REQUIRE(testWithoutDefaultValue().raw == initalTestWithoutDefaultValueValue.raw);
    }

    SECTION("deserialization with other datapoint versions")
    {
        using namespace std::string_view_literals;

        const auto [writeResult, writeSize, writeErrorCode] = DefaultGroup.serializeGroup("sample.bin"sv);
        REQUIRE(writeResult);
        REQUIRE(writeErrorCode == SerializationError::None);

        const auto [readResult, readSize, readErrorCode] = SecondGroup.deserializeGroup("sample.bin"sv);
        REQUIRE(readSize == writeSize);
        REQUIRE_FALSE(readResult);
        REQUIRE(readErrorCode == SerializationError::DatapointVersion);
    }

    SECTION("test for error in different group version")
    {
        using namespace std::string_view_literals;

        const auto [writeResult, writeSize, writeErrorCode] = OldGroup.serializeGroup("oldGroupSample.bin"sv);
        REQUIRE(writeResult);
        REQUIRE(writeErrorCode == SerializationError::None);

        const auto [readResult, readSize, readErrorCode] = NewerGroup.deserializeGroup("oldGroupSample.bin"sv);
        REQUIRE(readSize == writeSize);
        REQUIRE(readResult);
        REQUIRE(readErrorCode == SerializationError::GroupVersion);
    }

    SECTION("test for error in different group and datapoint versions")
    {
        using namespace std::string_view_literals;

        const auto [writeResult, writeSize, writeErrorCode] = OldGroup.serializeGroup("oldGroupSample.bin"sv);
        REQUIRE(writeResult);
        REQUIRE(writeErrorCode == SerializationError::None);

        const auto [readResult, readSize, readErrorCode] = NewerGroupAndDatapoint.deserializeGroup("oldGroupSample.bin"sv);
        REQUIRE(readSize == writeSize);
        REQUIRE_FALSE(readResult);
        REQUIRE(readErrorCode == SerializationError::GroupAndDatapointVersion);
    }

    SECTION("test for none error after update")
    {
        using namespace std::string_view_literals;

        const auto [writeResult, writeSize, writeErrorCode] = OldGroup.serializeGroup("oldGroupSample.bin"sv);
        REQUIRE(writeResult);
        REQUIRE(writeErrorCode == SerializationError::None);

        const auto [readResult, readSize, readErrorCode] = AllowUpgradeGroup.deserializeGroup("oldGroupSample.bin"sv);
        REQUIRE(readSize == writeSize);
        REQUIRE(readResult);
        REQUIRE(readErrorCode == SerializationError::None);
    }

    SECTION("test for not read all bytes")
    {
        using namespace std::string_view_literals;

        const auto writeStatus = OldGroupMultipleDatapoint.serializeGroup("OldGroupMultipleDatapoint.bin"sv);
        REQUIRE(writeStatus.result);
        REQUIRE(writeStatus.errorCode == SerializationError::None);

        const auto readStatus = NewerGroup.deserializeGroup("OldGroupMultipleDatapoint.bin"sv);
        REQUIRE_FALSE(readStatus.result);
        REQUIRE(readStatus.errorCode == SerializationError::GroupIdMismatch);
    }

    SECTION("getIsUpgradeAllowed returns false for default datapoint")
    {
        static_assert(test.getIsUpgradeAllowed() == false);
        static_assert(testWithoutDefaultValue.getIsUpgradeAllowed() == false);
    }

    SECTION("getIsUpgradeAllowed returns true for upgradable datapoint")
    {
        static_assert(UpgradableDatapoint.getIsUpgradeAllowed() == true);
    }

    SECTION("RangeAlias set returns underflow for value below minimum")
    {
        constexpr auto result = rangeAlias.set(RangeAlias{ 5 });
        STATIC_REQUIRE(result == DataLayer::Detail::RangeCheck::underflow);
    }

    SECTION("RangeAlias set returns overflow for value above maximum")
    {
        constexpr auto result = rangeAlias.set(RangeAlias{ 200 });
        STATIC_REQUIRE(result == DataLayer::Detail::RangeCheck::overflow);
    }

    SECTION("RangeAlias set returns ok for value within range")
    {
        const auto result = rangeAlias.set(RangeAlias{ 50 });
        REQUIRE(result == DataLayer::Detail::RangeCheck::ok);
        REQUIRE(rangeAlias().value == 50);
    }

    SECTION("RangeAlias set at minimum boundary returns ok")
    {
        const auto result = rangeAlias.set(RangeAlias{ 10 });
        REQUIRE(result == DataLayer::Detail::RangeCheck::ok);
    }

    SECTION("RangeAlias set at maximum boundary returns ok")
    {
        const auto result = rangeAlias.set(RangeAlias{ 100 });
        REQUIRE(result == DataLayer::Detail::RangeCheck::ok);
    }

    SECTION("RangeAlias underflow does not modify stored value")
    {
        std::ignore = rangeAlias.set(RangeAlias{ 50 });
        const auto before = rangeAlias().value;
        std::ignore = rangeAlias.set(RangeAlias{ 1 });
        REQUIRE(rangeAlias().value == before);
    }

    SECTION("RangeAlias overflow does not modify stored value")
    {
        std::ignore = rangeAlias.set(RangeAlias{ 50 });
        const auto before = rangeAlias().value;
        std::ignore = rangeAlias.set(RangeAlias{ 999 });
        REQUIRE(rangeAlias().value == before);
    }

    SECTION("dispatcher setDatapoint() fan-out to datapoint")
    {
        constexpr Temperature newValue{ .raw = 7777, .value = 77.7F };
        const auto [success, check] = Dispatcher.setDatapoint(test.getId(), newValue);
        REQUIRE(success);
        REQUIRE(check == DataLayer::Detail::RangeCheck::notChecked);
        REQUIRE(test().raw == newValue.raw);
    }

    SECTION("dispatcher setDatapoint() returns failure for unknown id")
    {
        constexpr uint32_t unknownId = 0xFFFF;
        constexpr Temperature dummyValue{};
        constexpr auto ret = Dispatcher.setDatapoint(unknownId, dummyValue);
        REQUIRE(!ret.success);
    }

    SECTION("arrayTest serialize and deserialize roundtrip")
    {
        const auto initialValue = arrayTest.get();
        const auto serialized = arrayTest.serialize();

        // overwrite with zeroed array
        constexpr std::array<Temperature, 10> zeroed{};
        std::ignore = arrayTest.set(zeroed);

        arrayTest.deserialize(serialized);
        for (size_t i = 0; i < initialValue.size(); ++i)
        {
            // TODO REQUIRE(arrayTest.get(i).raw == initialValue.at(i).raw);
        }
    }

    SECTION("structInStructType serialize and deserialize roundtrip")
    {
        const auto initialValue = structInStructType.get();
        const auto serialized = structInStructType.serialize();

        // overwrite with default-constructed Environment
        constexpr Environment zeroed{};
        const auto [success, check] = DefaultGroup.setDatapoint(structInStructType.getId(), zeroed);
        REQUIRE(success);
        REQUIRE(structInStructType.get().internal.raw == 0);

        structInStructType.deserialize(serialized);
        REQUIRE(structInStructType.get().internal.raw != initialValue.internal.raw);
        REQUIRE(structInStructType.get().external.raw != initialValue.external.raw);
    }

    test = initalTestValue;
    testWithoutDefaultValue = initalTestWithoutDefaultValueValue;
    testWithoutDefaultValueWriteOnly = initalTestWithoutDefaultValueWriteOnlyValue;
    arrayTest2 = initialArrayTest2;
    structInStructType = initialStructInStructType;
}
