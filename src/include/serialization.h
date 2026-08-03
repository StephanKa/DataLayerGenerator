#pragma once

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <helper.h>
#include <tuple>
#include <vector>

inline constexpr size_t MaxPersistenceFileSize = 16 * 1024 * 1024;

enum class SerializationError : uint8_t
{
    None,
    GroupVersion,
    DatapointVersion,
    GroupAndDatapointVersion,
    InvalidFormat,
    ChecksumMismatch,
    GroupIdMismatch
};

struct SerializationStatus
{
    bool result{ false };
    size_t size{ 0 };
    SerializationError errorCode{ SerializationError::None };
};

namespace DataLayer::Persistence
{
    inline constexpr std::array Magic{ 'D', 'L', 'G', '1' };
    inline constexpr uint16_t FormatVersion = 1;

    struct Header
    {
        std::array<char, 4> magic{ Magic };
        uint16_t formatVersion{ FormatVersion };
        uint16_t groupId{};
        Version groupVersion{};
        uint32_t checksum{};
    };

    struct RecordHeader
    {
        uint16_t dataPointId{};
        Version version{};
        uint32_t payloadSize{};
    };

    [[nodiscard]] inline uint32_t crc32(std::span<const std::byte> data) noexcept
    {
        uint32_t checksum = 0xFFFFFFFFU;
        for (const auto byte : data)
        {
            checksum ^= std::to_integer<uint8_t>(byte);
            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                checksum = (checksum >> 1U) ^ (0xEDB88320U & (0U - (checksum & 1U)));
            }
        }
        return ~checksum;
    }

    template<typename Value>
    void append(std::vector<std::byte> &output, const Value &value)
    {
        const auto bytes = std::as_bytes(std::span{ &value, 1 });
        output.insert(output.end(), bytes.begin(), bytes.end());
    }

    template<typename Value>
    [[nodiscard]] bool read(std::span<const std::byte> input, size_t &offset, Value &value) noexcept
    {
        if (offset > input.size() || input.size() - offset < sizeof(Value))
        {
            return false;
        }
        std::memcpy(&value, input.data() + offset, sizeof(Value));
        offset += sizeof(Value);
        return true;
    }
}// namespace DataLayer::Persistence

template<typename Data>
struct Serialization
{
    constexpr explicit Serialization(const DataLayer::Version &groupVersionInfo, uint16_t groupId, const std::filesystem::path &path, Data &input)
      : m_dataVariables(input), m_groupVersionInfo(groupVersionInfo), m_groupId(groupId), m_path(path)
    {}

    [[nodiscard]] SerializationStatus write()
    {
        std::vector<std::byte> records;
        bool success = true;
        std::apply([&](const auto &...dataPoints) { (appendRecord(records, dataPoints, success), ...); }, m_dataVariables);
        if (!success)
        {
            return { .result = false, .size = 0, .errorCode = SerializationError::InvalidFormat };
        }

        DataLayer::Persistence::Header header{ .groupId = m_groupId, .groupVersion = m_groupVersionInfo, .checksum = DataLayer::Persistence::crc32(records) };
        std::vector<std::byte> output;
        DataLayer::Persistence::append(output, header);
        output.insert(output.end(), records.begin(), records.end());

        const auto temporaryPath = m_path.string() + ".tmp";
        {
            std::ofstream outputFile(temporaryPath, std::ios::binary | std::ios::trunc);
            outputFile.write(reinterpret_cast<const char *>(output.data()), static_cast<std::streamsize>(output.size()));
            outputFile.flush();
            if (outputFile.fail())
            {
                return { .result = false, .size = output.size(), .errorCode = SerializationError::InvalidFormat };
            }
        }

        std::error_code error;
        std::filesystem::rename(temporaryPath, m_path, error);
        if (error)
        {
            std::filesystem::remove(m_path, error);
            error.clear();
            std::filesystem::rename(temporaryPath, m_path, error);
        }
        return { .result = !error, .size = output.size(), .errorCode = error ? SerializationError::InvalidFormat : SerializationError::None };
    }

  private:
    static void appendRecord(std::vector<std::byte> &records, const auto &dataPoint, bool &success)
    {
        const auto value = dataPoint();
        using Value = std::remove_cvref_t<decltype(value)>;
        if constexpr (!std::is_trivially_copyable_v<Value>)
        {
            success = false;
            return;
        }
        const DataLayer::Persistence::RecordHeader header{ .dataPointId = dataPoint.getId(), .version = dataPoint.getVersion(), .payloadSize = sizeof(Value) };
        DataLayer::Persistence::append(records, header);
        DataLayer::Persistence::append(records, value);
    }

    Data &m_dataVariables;
    DataLayer::Version m_groupVersionInfo;
    uint16_t m_groupId;
    std::filesystem::path m_path;
};

template<typename Data>
struct Deserialization
{
    constexpr explicit Deserialization(const DataLayer::Version &groupVersionInfo, uint16_t groupId, const std::filesystem::path &path, Data &input, bool allowUpgrade)
      : m_dataVariables(input), m_groupVersionInfo(groupVersionInfo), m_groupId(groupId), m_path(path), m_allowUpgrade(allowUpgrade)
    {}

    [[nodiscard]] SerializationStatus read()
    {
        std::ifstream inputFile(m_path, std::ios::binary | std::ios::ate);
        if (!inputFile)
        {
            return { .result = false, .size = 0, .errorCode = SerializationError::InvalidFormat };
        }
        const auto fileSize = static_cast<size_t>(inputFile.tellg());
        if (fileSize < sizeof(DataLayer::Persistence::Header) || fileSize > MaxPersistenceFileSize)
        {
            return { .result = false, .size = fileSize, .errorCode = SerializationError::InvalidFormat };
        }

        std::vector<std::byte> input(fileSize);
        inputFile.seekg(0);
        inputFile.read(reinterpret_cast<char *>(input.data()), static_cast<std::streamsize>(input.size()));
        if (inputFile.fail())
        {
            return { .result = false, .size = 0, .errorCode = SerializationError::InvalidFormat };
        }

        DataLayer::Persistence::Header header{};
        size_t offset = 0;
        if (!DataLayer::Persistence::read(std::span{ input }, offset, header) || header.magic != DataLayer::Persistence::Magic
            || header.formatVersion != DataLayer::Persistence::FormatVersion)
        {
            return { .result = false, .size = input.size(), .errorCode = SerializationError::InvalidFormat };
        }
        if (header.groupId != m_groupId)
        {
            return { .result = false, .size = input.size(), .errorCode = SerializationError::GroupIdMismatch };
        }

        const auto records = std::span<const std::byte>{ input }.subspan(offset);
        if (header.checksum != DataLayer::Persistence::crc32(records))
        {
            return { .result = false, .size = input.size(), .errorCode = SerializationError::ChecksumMismatch };
        }

        SerializationError error = (m_groupVersionInfo > header.groupVersion && !m_allowUpgrade) ? SerializationError::GroupVersion : SerializationError::None;
        bool success = true;
        while (offset < input.size())
        {
            DataLayer::Persistence::RecordHeader record{};
            if (!DataLayer::Persistence::read(std::span{ input }, offset, record) || input.size() - offset < record.payloadSize)
            {
                return { .result = false, .size = offset, .errorCode = SerializationError::InvalidFormat };
            }
            const auto payload = std::span<const std::byte>{ input }.subspan(offset, record.payloadSize);
            offset += record.payloadSize;
            std::apply([&](auto &...dataPoints) { (readRecord(dataPoints, record, payload, error, success), ...); }, m_dataVariables);
        }
        return { .result = success, .size = input.size(), .errorCode = error };
    }

  private:
    static void readRecord(auto &dataPoint, const DataLayer::Persistence::RecordHeader &record, std::span<const std::byte> payload, SerializationError &error, bool &success)
    {
        if (!dataPoint.matchesId(record.dataPointId))
        {
            return;
        }

        using Value = std::remove_cvref_t<decltype(dataPoint())>;
        if constexpr (!std::is_trivially_copyable_v<Value>)
        {
            success = false;
            error = SerializationError::InvalidFormat;
            return;
        }

        Value value{};
        const bool requiresUpgrade = dataPoint.getVersion() > record.version;
        if (requiresUpgrade && !dataPoint.getIsUpgradeAllowed())
        {
            success = false;
            setVersionError(error);
            return;
        }
        if (payload.size() != sizeof(Value))
        {
            if (!requiresUpgrade || !dataPoint.tryMigrate(record.version, payload, value))
            {
                success = false;
                error = requiresUpgrade ? SerializationError::DatapointVersion : SerializationError::InvalidFormat;
                return;
            }
        }
        else
        {
            std::memcpy(&value, payload.data(), sizeof(Value));
        }
        dataPoint = value;
    }

    static void setVersionError(SerializationError &error)
    {
        error = error == SerializationError::GroupVersion ? SerializationError::GroupAndDatapointVersion : SerializationError::DatapointVersion;
    }

    Data &m_dataVariables;
    DataLayer::Version m_groupVersionInfo;
    uint16_t m_groupId;
    std::filesystem::path m_path;
    bool m_allowUpgrade;
};
