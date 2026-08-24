#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <helper.h>
#include <ranges>
#include <string_view>

namespace DataLayer
{
    // group definitions
    template<uint16_t BaseId, FixedString Name, bool AllowUpgrade, PersistenceType persistence = PersistenceType::None, auto Version = Version{ 0, 0, 0 }>
    struct GroupInfo
    {
        using ChangeCallback = void (*)(uint32_t dataPointId, void *context) noexcept;

      private:
        struct ChangeObserver
        {
            ChangeCallback callback{};
            void *context{};
        };

      public:
        static constexpr std::size_t ObserverCapacity{ 4U };
        constexpr static PersistenceType persist{ persistence };
        constexpr static uint16_t baseId{ BaseId };
        constexpr static auto version{ Version };
        constexpr static std::string_view name{ Name };
        constexpr static bool allowUpgrade{ AllowUpgrade };

        [[nodiscard]] static constexpr bool addChangeCallback(ChangeCallback callback, void *context = nullptr) noexcept
        {
            if (callback == nullptr)
            {
                return false;
            }

            const auto existingObserver =
              std::ranges::find_if(m_changeCallbacks, [callback, context](const ChangeObserver &observer) { return observer.callback == callback && observer.context == context; });
            if (existingObserver != m_changeCallbacks.end())
            {
                return true;
            }

            const auto availableObserver = std::ranges::find_if(m_changeCallbacks, [](const ChangeObserver &observer) { return observer.callback == nullptr; });
            if (availableObserver == m_changeCallbacks.end())
            {
                return false;
            }
            *availableObserver = ChangeObserver{ .callback = callback, .context = context };
            return true;
        }

        [[nodiscard]] static constexpr bool removeChangeCallback(ChangeCallback callback, void *context = nullptr) noexcept
        {
            const auto observer = std::ranges::find_if(
              m_changeCallbacks, [callback, context](const ChangeObserver &candidate) { return candidate.callback == callback && candidate.context == context; });
            if (observer == m_changeCallbacks.end())
            {
                return false;
            }
            *observer = {};
            return true;
        }

        static constexpr void clearChangeCallbacks() noexcept
        {
            m_changeCallbacks = {};
        }

        [[nodiscard]] static constexpr std::size_t observerStorageBytes() noexcept
        {
            return sizeof(m_changeCallbacks);
        }

        static constexpr void notifyChange(uint32_t dataPointId) noexcept
        {
            std::ranges::for_each(m_changeCallbacks | std::views::filter([](const ChangeObserver &observer) { return observer.callback != nullptr; }),
              [dataPointId](const ChangeObserver &observer) { observer.callback(dataPointId, observer.context); });
        }

      private:
        inline static std::array<ChangeObserver, ObserverCapacity> m_changeCallbacks{};
    };
}// namespace DataLayer
