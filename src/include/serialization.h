#pragma once

#include <array>
#include <concepts>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <helper.h>
#include <limits>
#include <optional>
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

    template<typename Backend>
    concept StorageBackend = requires(Backend &backend, const Backend &readBackend, std::span<const std::byte> output, std::span<std::byte> input) {
        { backend.write(output) } -> std::convertible_to<bool>;
        { readBackend.size() } -> std::same_as<std::optional<size_t>>;
        { readBackend.read(input) } -> std::convertible_to<bool>;
    };

    class FileBackend
    {
      public:
        explicit FileBackend(std::filesystem::path path) : m_path(std::move(path))
        {}

        [[nodiscard]] bool write(const std::span<const std::byte> output)
        {
            const auto temporaryPath = m_path.string() + ".tmp";
            {
                std::ofstream outputFile(temporaryPath, std::ios::binary | std::ios::trunc);
                outputFile.write(reinterpret_cast<const char *>(output.data()), static_cast<std::streamsize>(output.size()));
                outputFile.flush();
                if (outputFile.fail())
                {
                    return false;
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
            return !error;
        }

        [[nodiscard]] std::optional<size_t> size() const
        {
            std::error_code error;
            const auto result = std::filesystem::file_size(m_path, error);
            if (error || result > std::numeric_limits<size_t>::max())
            {
                return std::nullopt;
            }
            return static_cast<size_t>(result);
        }

        [[nodiscard]] bool read(const std::span<std::byte> input) const
        {
            std::ifstream inputFile(m_path, std::ios::binary);
            inputFile.read(reinterpret_cast<char *>(input.data()), static_cast<std::streamsize>(input.size()));
            return !inputFile.fail();
        }

      private:
        std::filesystem::path m_path;
    };

    inline void appendRecord(std::vector<std::byte> &records, const auto &dataPoint, bool &success)
    {
        const auto value = dataPoint();
        using Value = std::remove_cvref_t<decltype(value)>;
        if constexpr (!std::is_trivially_copyable_v<Value>)
        {
            success = false;
            return;
        }
        const RecordHeader header{ .dataPointId = dataPoint.getId(), .version = dataPoint.getVersion(), .payloadSize = sizeof(Value) };
        append(records, header);
        append(records, value);
    }

    inline void setVersionError(SerializationError &error)
    {
        error = error == SerializationError::GroupVersion ? SerializationError::GroupAndDatapointVersion : SerializationError::DatapointVersion;
    }

    inline void readRecord(auto &dataPoint, const RecordHeader &record, const std::span<const std::byte> payload, SerializationError &error, bool &success)
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

    template<StorageBackend Backend, typename Data>
    [[nodiscard]] SerializationStatus serialize(Backend &backend, const Version &groupVersionInfo, const uint16_t groupId, Data &input)
    {
        std::vector<std::byte> records;
        bool success = true;
        std::apply([&](const auto &...dataPoints) { (appendRecord(records, dataPoints, success), ...); }, input);
        if (!success)
        {
            return { .result = false, .size = 0, .errorCode = SerializationError::InvalidFormat };
        }

        Header header{ .groupId = groupId, .groupVersion = groupVersionInfo, .checksum = crc32(records) };
        std::vector<std::byte> output;
        append(output, header);
        output.insert(output.end(), records.begin(), records.end());
        const bool writeSuccess = backend.write(std::span<const std::byte>{ output });
        return { .result = writeSuccess, .size = output.size(), .errorCode = writeSuccess ? SerializationError::None : SerializationError::InvalidFormat };
    }

    template<StorageBackend Backend, typename Data>
    [[nodiscard]] SerializationStatus deserialize(const Backend &backend, const Version &groupVersionInfo, const uint16_t groupId, Data &input, const bool allowUpgrade)
    {
        const auto storedSize = backend.size();
        if (!storedSize || *storedSize < sizeof(Header) || *storedSize > MaxPersistenceFileSize)
        {
            return { .result = false, .size = storedSize.value_or(0), .errorCode = SerializationError::InvalidFormat };
        }

        std::vector<std::byte> inputBytes(*storedSize);
        if (!backend.read(inputBytes))
        {
            return { .result = false, .size = 0, .errorCode = SerializationError::InvalidFormat };
        }

        Header header{};
        size_t offset = 0;
        if (!read(std::span{ inputBytes }, offset, header) || header.magic != Magic || header.formatVersion != FormatVersion)
        {
            return { .result = false, .size = inputBytes.size(), .errorCode = SerializationError::InvalidFormat };
        }
        if (header.groupId != groupId)
        {
            return { .result = false, .size = inputBytes.size(), .errorCode = SerializationError::GroupIdMismatch };
        }

        const auto records = std::span<const std::byte>{ inputBytes }.subspan(offset);
        if (header.checksum != crc32(records))
        {
            return { .result = false, .size = inputBytes.size(), .errorCode = SerializationError::ChecksumMismatch };
        }

        SerializationError error = (groupVersionInfo > header.groupVersion && !allowUpgrade) ? SerializationError::GroupVersion : SerializationError::None;
        bool success = true;
        while (offset < inputBytes.size())
        {
            RecordHeader record{};
            if (!read(std::span{ inputBytes }, offset, record) || inputBytes.size() - offset < record.payloadSize)
            {
                return { .result = false, .size = offset, .errorCode = SerializationError::InvalidFormat };
            }
            const auto payload = std::span<const std::byte>{ inputBytes }.subspan(offset, record.payloadSize);
            offset += record.payloadSize;
            std::apply([&](auto &...dataPoints) { (readRecord(dataPoints, record, payload, error, success), ...); }, input);
        }
        return { .result = success, .size = inputBytes.size(), .errorCode = error };
    }
}// namespace DataLayer::Persistence

template<typename Data>
struct Serialization
{
    constexpr explicit Serialization(const DataLayer::Version &groupVersionInfo, uint16_t groupId, const std::filesystem::path &path, Data &input)
      : m_dataVariables(input), m_groupVersionInfo(groupVersionInfo), m_groupId(groupId), m_backend(path)
    {}

    [[nodiscard]] SerializationStatus write()
    {
        return DataLayer::Persistence::serialize(m_backend, m_groupVersionInfo, m_groupId, m_dataVariables);
    }

  private:
    Data &m_dataVariables;
    DataLayer::Version m_groupVersionInfo;
    uint16_t m_groupId;
    DataLayer::Persistence::FileBackend m_backend;
};

template<typename Data>
struct Deserialization
{
    constexpr explicit Deserialization(const DataLayer::Version &groupVersionInfo, uint16_t groupId, const std::filesystem::path &path, Data &input, bool allowUpgrade)
      : m_dataVariables(input), m_groupVersionInfo(groupVersionInfo), m_groupId(groupId), m_backend(path), m_allowUpgrade(allowUpgrade)
    {}

    [[nodiscard]] SerializationStatus read()
    {
        return DataLayer::Persistence::deserialize(m_backend, m_groupVersionInfo, m_groupId, m_dataVariables, m_allowUpgrade);
    }

  private:
    Data &m_dataVariables;
    DataLayer::Version m_groupVersionInfo;
    uint16_t m_groupId;
    DataLayer::Persistence::FileBackend m_backend;
    bool m_allowUpgrade;
};
