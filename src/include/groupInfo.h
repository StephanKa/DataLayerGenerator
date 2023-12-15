#pragma once

#include <cstdint>
#include <helper.h>

namespace DataLayer {
// group definitions
template<uint16_t BaseId, FixedString Name, bool AllowUpgrade, Persistance persistence = Persistance::None, auto Version = Version{ 0, 0, 0 }>
struct GroupInfo
{
    constexpr static Persistance persist{ persistence };
    constexpr static uint16_t baseId{ BaseId };
    constexpr static auto version{ Version };
    static constexpr char const *name = Name;
    constexpr static bool allowUpgrade = AllowUpgrade;
};
}// namespace DataLayer
