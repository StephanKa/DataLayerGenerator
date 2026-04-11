#pragma once
#include <cstdint>
#include <helper.h>
#include <string_view>

namespace DataLayer
{
    // group definitions
    template<uint16_t BaseId, FixedString Name, bool AllowUpgrade, PersistenceType persistence = PersistenceType::None, auto Version = Version{ 0, 0, 0 }>
    struct GroupInfo
    {
        constexpr static PersistenceType persist{ persistence };
        constexpr static uint16_t baseId{ BaseId };
        constexpr static auto version{ Version };
        constexpr static std::string_view name{ Name };
        constexpr static bool allowUpgrade{ AllowUpgrade };
    };
}// namespace DataLayer
