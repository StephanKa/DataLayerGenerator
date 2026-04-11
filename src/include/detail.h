#pragma once
#include <array>
#include <concepts>
#include <cstdint>
#include <helper.h>
#include <variant>

namespace DataLayer::Detail
{
    template<typename T, size_t N>
    [[nodiscard]] constexpr std::array<T, N> make_array(T value) noexcept
    {
        std::array<T, N> temp{};
        for (auto &val : temp)
        {
            val = value;
        }
        return temp;
    }

    template<typename Type>
    concept IsArray = requires { typename Type::value_type; } && !std::is_same_v<std::decay_t<Type>, std::string>;

    template<typename Type>
    struct BaseType
    {
        constexpr BaseType() = default;

        constexpr BaseType(const auto &val) : value{ static_cast<Type>(val) }
        {}

        constexpr Type operator()() const noexcept
        {
            return value;
        }

        Type value{};
    };

    enum class RangeCheck : std::uint8_t
    {
        underflow,
        overflow,
        ok,
        notChecked
    };

    struct CheckResult
    {
        bool success{ false };
        RangeCheck check{ RangeCheck::notChecked };
    };

    template<typename Type>
    concept hasRange = requires {
        Type::Minimum;
        Type::Maximum;
    };

    template<typename T>
    [[nodiscard]] auto checkValue(T value) noexcept
    {
        using Type = std::remove_cvref_t<T>;
        if constexpr (hasRange<Type>)
        {
            using UnderlyingType = Type::Type;
            using RetType = std::variant<RangeCheck, UnderlyingType>;
            if (value() < Type::Minimum)
            {
                return RetType{ RangeCheck::underflow };
            }
            if (value() > Type::Maximum)
            {
                return RetType{ RangeCheck::overflow };
            }
            return RetType{ value() };
        }
        else
        {
            using RetType = std::variant<RangeCheck, T>;
            return RetType{ value };
        }
    }
}// namespace DataLayer::Detail
