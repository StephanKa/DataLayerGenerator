#pragma once

#include <array>
#include <cstdint>
#include <helper.h>

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
}// namespace DataLayer::Persistence
