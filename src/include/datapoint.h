#pragma once
#include <cstdint>
#include <cstring>
#include <groupInfo.h>
#include <span>
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
        using ChangeCallback = void (*)(const T &value, void *context) noexcept;

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

        [[nodiscard]] static bool tryMigrate(const Version &sourceVersion, std::span<const std::byte> source, T &destination) noexcept
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
            m_changeCallback = callback;
            m_changeContext = context;
        }

        constexpr void clearChangeCallback() noexcept
        {
            m_changeCallback = nullptr;
            m_changeContext = nullptr;
        }

      private:
        constexpr auto setValue(const T &value)
        {
            const auto result = Detail::checkValue(value);
            if (result.has_value())
            {
                m_value = *result;
                if (m_changeCallback != nullptr)
                {
                    m_changeCallback(m_value, m_changeContext);
                }
                return Detail::RangeCheck::ok;
            }
            return result.error();
        }

        T m_value{};
        ChangeCallback m_changeCallback{ nullptr };
        void *m_changeContext{ nullptr };
    };
}// namespace DataLayer
