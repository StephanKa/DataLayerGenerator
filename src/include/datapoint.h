#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <detail.h>
#include <groupInfo.h>
#include <ranges>
#include <span>
#include <trace.h>
#include <type_traits>

namespace DataLayer
{
    // data point definition

    template<typename T,
      GroupInfo group,
      uint16_t id,
      typename Access,
      auto Version = Version{ 0, 0, 0 },
      FixedString Name = { "" },
      bool AllowUpgrade = false,
      auto Migration = nullptr,
      uint16_t... LegacyIds>
    class DataPoint
    {
      public:
        using ValueType = T;
        using ChangeCallback = void (*)(const T &value, void *context) noexcept;

      private:
        struct ChangeObserver
        {
            ChangeCallback callback{};
            void *context{};
        };

      public:
        static constexpr std::size_t ObserverCapacity{ 4U };

        constexpr static Access TypeAccess{};

        consteval DataPoint() = default;

        consteval explicit DataPoint(T value) : m_value(value)
        {}

        static constexpr std::string_view name{ Name };

        [[nodiscard]] constexpr static uint16_t getId() noexcept
        {
            return group.baseId + id;
        }

        [[nodiscard]] constexpr static bool matchesId(uint32_t dataPointId) noexcept
        {
            return dataPointId == getId() || ((dataPointId == group.baseId + LegacyIds) || ...);
        }

        [[nodiscard]] constexpr static auto getVersion() noexcept
        {
            return Version;
        }

        // function to read everytime
        [[nodiscard]] constexpr T operator()() const noexcept
        {
            return m_value;
        }

#ifdef DATALAYER_DEBUG_EXPORT
        [[nodiscard]] constexpr const T *debugValueAddress() const noexcept
        {
            return &m_value;
        }
#endif

        // function to write anyway
        constexpr DataPoint &operator=(const T &value)
        {
            setValue(value);
            return *this;
        }

        // function that will be restricted by READ and READ_WRITE access
        template<typename A = Access>
            requires Helper::ReadConcept<A>
        [[nodiscard]] T &get() noexcept
        {
            return m_value;
        }

        // function that will be restricted by READ and READ_WRITE access
        template<typename A = Access>
            requires Helper::ReadConcept<A> && Detail::IsArray<T>
        [[nodiscard]] constexpr auto &get(size_t index)
        {
            return m_value.at(index);
        }

        template<typename A = Access>
            requires Helper::ReadConcept<A>
        [[nodiscard]] auto serialize()
        {
            static_assert(std::is_trivially_copyable_v<T>);
            return std::as_bytes(std::span{ &m_value, 1 });
        }

        // function that will be restricted by WRITE and READ_WRITE access
        template<typename A = Access>
            requires Helper::WriteConcept<A>
        [[nodiscard]] constexpr auto set(const T &value) noexcept
        {
            return setValue(value);
        }

        // function that will be restricted by WRITE and READ_WRITE access
        template<typename A = Access>
            requires Helper::WriteConcept<A> && Detail::IsArray<T>
        constexpr void set(size_t index, const auto &value)
        {
            m_value.at(index) = value;
        }

        template<typename A = Access>
            requires Helper::WriteConcept<A>
        void deserialize(std::span<const std::byte> bytes)
        {
            if (bytes.size() == sizeof(T))
            {
                std::memcpy(&m_value, bytes.data(), sizeof(T));
            }
        }

        template<typename Type = T>
            requires Detail::IsArray<Type>
        [[nodiscard]] static constexpr auto size() noexcept
        {
            return Type{}.size();
        }

        [[nodiscard]] static constexpr bool getIsUpgradeAllowed() noexcept
        {
            return AllowUpgrade;
        }

        [[nodiscard]] static bool tryMigrate(const DataLayer::Version &sourceVersion, std::span<const std::byte> source, T &destination) noexcept
        {
            if constexpr (!std::is_same_v<decltype(Migration), std::nullptr_t>)
            {
                static_assert(std::is_trivially_copyable_v<T>);
                return Migration(sourceVersion, source, std::as_writable_bytes(std::span{ &destination, 1 }));
            }
            return false;
        }

        constexpr void setChangeCallback(ChangeCallback callback, void *context = nullptr) noexcept
        {
            clearChangeCallbacks();
            std::ignore = addChangeCallback(callback, context);
        }

        constexpr void clearChangeCallback() noexcept
        {
            clearChangeCallbacks();
        }

        [[nodiscard]] constexpr bool addChangeCallback(ChangeCallback callback, void *context = nullptr) noexcept
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

        [[nodiscard]] constexpr bool removeChangeCallback(ChangeCallback callback, void *context = nullptr) noexcept
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

        constexpr void clearChangeCallbacks() noexcept
        {
            m_changeCallbacks = {};
        }

      private:
        constexpr auto setValue(const T &value)
        {
            const auto result = Detail::checkValue(value);
            if (result.has_value())
            {
                m_value = *result;
                std::ranges::for_each(m_changeCallbacks | std::views::filter([](const ChangeObserver &observer) { return observer.callback != nullptr; }),
                  [this](const ChangeObserver &observer) { observer.callback(m_value, observer.context); });
                return Detail::RangeCheck::ok;
            }
#ifdef USE_TRACE_HOOKS
            if (!std::is_constant_evaluated())
            {
                Trace::emit({ .kind = Trace::EventKind::RangeRejected,
                  .source = Trace::Source::Core,
                  .identifier = getId(),
                  .detail = static_cast<uint32_t>(result.error()),
                  .success = false });
            }
#endif
            return result.error();
        }

        T m_value{};
        std::array<ChangeObserver, ObserverCapacity> m_changeCallbacks{};
    };
}// namespace DataLayer
