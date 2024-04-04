#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <helper.h>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

enum class SerializationError : uint8_t
{
    None,
    GroupVersion,
    DatapointVersion,
    GroupAndDatapointVersion,
    NotAllBytesRead
};

struct SerializationStatus
{
    bool result{ false };
    size_t size{ 0 };
    SerializationError errorCode{ SerializationError::None };
};

template<typename Type>
concept IsContainer = requires(Type val)
{
    val.size();
};

template<typename Type>
concept IsString = std::is_same_v<char *, std::decay_t<Type>> || std::is_same_v<const char *, std::decay_t<Type>> || std::is_same_v<std::string, std::decay_t<Type>>;

template<typename Data>
struct Serialization
{

    constexpr explicit Serialization(const Version &groupVersionInfo, std::string_view path, Data &input)
      : m_dataVariables(input), m_ofileNonPOD(path.data(), std::ios::binary), m_groupVersionInfo(groupVersionInfo)
    {}

    ~Serialization()
    {
        close();
    }

    Serialization(const Serialization &) = default;
    Serialization &operator=(const Serialization &) = default;
    Serialization(Serialization &&) noexcept = default;
    Serialization &operator=(Serialization &&) noexcept = default;

    [[nodiscard]] SerializationStatus write()
    {
        bool ret = false;
        size_t size = 0;

        m_ofileNonPOD.write(reinterpret_cast<const char *>(&m_groupVersionInfo), sizeof(m_groupVersionInfo));
        size += sizeof(m_groupVersionInfo);

        return std::apply(
          [this, &ret, &size](auto &...args) {
              ((writeImpl(m_ofileNonPOD, args, ret, size)) || ... || true);

              return SerializationStatus{ ret, size };
          },
          m_dataVariables);
    }

    void close()
    {
        if (m_ofileNonPOD.is_open()) {
            m_ofileNonPOD.close();
        }
    }

  private:
    bool static writeImpl(std::ofstream &ofile, auto &val, bool &ret, size_t &size)
    {
        ret = !ofile.fail();
        if (ofile.fail()) {
            return true;
        }

        const auto version = val.getVersion();
        ofile.write(reinterpret_cast<const char *>(&version), sizeof(version));
        size += sizeof(version);
        const auto value = val();

        if constexpr (IsString<std::remove_cvref_t<decltype(value)>>) {
            const auto valueSize = value.size();
            ofile.write(reinterpret_cast<const char *>(&valueSize), sizeof(valueSize));
            size += sizeof(valueSize);
            ofile.write(reinterpret_cast<const char *>(value.data()), valueSize);
            size += valueSize;
        } else if constexpr (IsContainer<std::remove_cvref_t<decltype(value)>>) {
            constexpr auto valueSize = sizeof(std::remove_extent_t<decltype(value)>);
            ofile.write(reinterpret_cast<const char *>(value.data()), valueSize);
            size += valueSize;
        } else {
            ofile.write(reinterpret_cast<const char *>(&value), sizeof(value));
            size += sizeof(value);
        }
        return false;
    }

    Data &m_dataVariables;
    std::ofstream m_ofileNonPOD;
    Version m_groupVersionInfo;
};

template<typename Data>
struct Deserialization
{
    constexpr explicit Deserialization(const Version &groupVersionInfo, std::string_view path, Data &input, bool allowUpgrade)
      : m_dataVariables(input), m_ifileNonPOD(path.data(), std::ios::binary), m_fileSize(std::filesystem::file_size(path)), m_groupVersionInfo(groupVersionInfo),
        m_allowUpgrade(allowUpgrade)
    {}

    ~Deserialization()
    {
        if (m_ifileNonPOD.is_open()) {
            m_ifileNonPOD.close();
        }
    }

    Deserialization(const Deserialization &) = default;
    Deserialization &operator=(const Deserialization &) = default;
    Deserialization(Deserialization &&) noexcept = default;
    Deserialization &operator=(Deserialization &&) noexcept = default;

    [[nodiscard]] SerializationStatus read()
    {
        bool ret = true;
        size_t size = 0;
        SerializationError error = SerializationError::None;

        Version groupVersionTemp;
        m_ifileNonPOD.read(reinterpret_cast<char *>(&groupVersionTemp), sizeof(groupVersionTemp));
        size += sizeof(groupVersionTemp);
        if ((groupVersionTemp > m_groupVersionInfo) && !m_allowUpgrade) {
            error = SerializationError::GroupVersion;
        }
        return std::apply(
          [this, &ret, &size, &error](auto &...args) {
              ((readImpl(m_ifileNonPOD, args, ret, size, m_fileSize, error)) || ... || true);

              if ((size < m_fileSize) && (error == SerializationError::None)) {
                  error = SerializationError::NotAllBytesRead;
              }
              return SerializationStatus{ ret, size, error };
          },
          m_dataVariables);
    }

  private:
    bool static readImpl(std::ifstream &ifile, auto &val, bool &ret, size_t &size, const size_t fileSize, SerializationError &error)
    {
        ret &= !ifile.fail();
        if (ifile.fail()) {
            return true;
        }

        Version temp;
        ifile.read(reinterpret_cast<char *>(&temp), sizeof(temp));
        size += sizeof(temp);
        auto tempValue = val();

        if constexpr (IsString<std::remove_cvref_t<decltype(tempValue)>>) {
            size_t valueSize{ 0 };
            size += sizeof(valueSize);
            if (fileSize < size) {
                return false;
            }
            ifile.read(reinterpret_cast<char *>(&valueSize), sizeof(valueSize));
            tempValue.reserve(valueSize);
            size += valueSize;
            if (fileSize < size) {
                return false;
            }
            ifile.read(reinterpret_cast<char *>(&tempValue.front()), valueSize);
        } else if constexpr (IsContainer<std::remove_cvref_t<decltype(tempValue)>>) {
            constexpr auto valueSize = sizeof(std::remove_extent_t<decltype(tempValue)>);
            size += valueSize;
            if (fileSize < size) {
                return false;
            }
            ifile.read(reinterpret_cast<char *>(&tempValue.front()), valueSize);
        } else {
            size += sizeof(tempValue);
            if (fileSize < size) {
                return false;
            }
            ifile.read(reinterpret_cast<char *>(&tempValue), sizeof(tempValue));
        }
        if ((temp > val.getVersion()) && !val.getIsUpgradeAllowed()) {
            ret = false;
            switch (error) {
            case SerializationError::None: {
                error = SerializationError::DatapointVersion;
                break;
            }
            case SerializationError::GroupVersion: {
                error = SerializationError::GroupAndDatapointVersion;
                break;
            }
            default:
                break;
            }
            return false;
        }
        val = tempValue;
        return false;
    }

    Data &m_dataVariables;
    std::ifstream m_ifileNonPOD;
    size_t m_fileSize;
    Version m_groupVersionInfo;
    bool m_allowUpgrade;
};
