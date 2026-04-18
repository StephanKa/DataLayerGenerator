#pragma once
#include <cstdint>
#include <groupInfo.h>
#include <span>
#include <cstring>

namespace DataLayer
{
    // data point definition

    template<typename T, GroupInfo group, uint16_t id, typename Access, auto Version = Version{ 0, 0, 0 }, FixedString Name = { "" }, bool AllowUpgrade = false>
    class DataPoint
    {
      public:
        constexpr static Access TypeAccess{};

        consteval DataPoint() = default;

        consteval explicit DataPoint(T value) : m_value(value)
        {}

        static constexpr std::string_view name{ Name };

        [[nodiscard]] constexpr static uint16_t getId() noexcept
        {
            return group.baseId + id;
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
        [[nodiscard]] constexpr auto size() noexcept
        {
            return m_value.size();
        }

        [[nodiscard]] static constexpr bool getIsUpgradeAllowed() noexcept
        {
            return AllowUpgrade;
        }

      private:
        constexpr auto setValue(const T &value)
        {
            const auto checkValue = Detail::checkValue(value);
            auto val = Detail::RangeCheck::ok;
            std::visit(Helper::overloaded{ [&](const Detail::RangeCheck check) { val = check; }, [&](const auto &arg) { m_value = arg; } }, checkValue);
            return val;
        }

        T m_value{};
    };
}// namespace DataLayer
