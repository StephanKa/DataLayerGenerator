#pragma once
#include <array>
#include <concepts>
#include <cstdint>
#include <expected>
#include <helper.h>
#include <string>
#include <string_view>

namespace DataLayer::Detail
{
    template<typename T, size_t N>
    [[nodiscard]] constexpr std::array<T, N> make_array(T value) noexcept
    {
        std::array<T, N> temp{};
        temp.fill(value);
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
        stepMismatch,
        notAllowed,
        lengthUnderflow,
        lengthOverflow,
        patternMismatch,
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

    template<typename Type>
    concept hasStep = requires { Type::Step; };

    template<typename Type>
    concept hasAllowedValues = requires { Type::AllowedValues; };

    template<typename Type>
    concept hasMinimumLength = requires { Type::MinimumLength; };

    template<typename Type>
    concept hasMaximumLength = requires { Type::MaximumLength; };

    template<typename Type>
    concept hasPattern = requires { Type::Pattern; };

    [[nodiscard]] constexpr bool matchesGlob(std::string_view value, std::string_view pattern) noexcept
    {
        size_t valueIndex{};
        size_t patternIndex{};
        size_t wildcardIndex{ std::string_view::npos };
        size_t wildcardValueIndex{};
        while (valueIndex < value.size())
        {
            if (patternIndex < pattern.size() && (pattern[patternIndex] == '?' || pattern[patternIndex] == value[valueIndex]))
            {
                ++valueIndex;
                ++patternIndex;
            }
            else if (patternIndex < pattern.size() && pattern[patternIndex] == '*')
            {
                wildcardIndex = patternIndex++;
                wildcardValueIndex = valueIndex;
            }
            else if (wildcardIndex != std::string_view::npos)
            {
                patternIndex = wildcardIndex + 1U;
                valueIndex = ++wildcardValueIndex;
            }
            else
            {
                return false;
            }
        }
        while (patternIndex < pattern.size() && pattern[patternIndex] == '*')
        {
            ++patternIndex;
        }
        return patternIndex == pattern.size();
    }

    template<typename T>
    [[nodiscard]] constexpr auto checkValue(T value) noexcept
    {
        using Type = std::remove_cvref_t<T>;
        if constexpr (hasRange<Type>)
        {
            using RetType = std::expected<T, RangeCheck>;
            if (value() < Type::Minimum)
            {
                return RetType{ std::unexpected(RangeCheck::underflow) };
            }
            if (value() > Type::Maximum)
            {
                return RetType{ std::unexpected(RangeCheck::overflow) };
            }
            if constexpr (hasStep<Type>)
            {
                if ((value() - Type::Minimum) % Type::Step != 0)
                {
                    return RetType{ std::unexpected(RangeCheck::stepMismatch) };
                }
            }
            if constexpr (hasAllowedValues<Type>)
            {
                for (const auto allowed : Type::AllowedValues)
                {
                    if (value() == allowed)
                    {
                        return RetType{ value };
                    }
                }
                return RetType{ std::unexpected(RangeCheck::notAllowed) };
            }
            return RetType{ value };
        }
        else
        {
            using RetType = std::expected<T, RangeCheck>;
            if constexpr (hasAllowedValues<Type>)
            {
                for (const auto allowed : Type::AllowedValues)
                {
                    if (value() == allowed)
                    {
                        return RetType{ value };
                    }
                }
                return RetType{ std::unexpected(RangeCheck::notAllowed) };
            }
            if constexpr (hasMinimumLength<Type>)
            {
                if (value().size() < Type::MinimumLength)
                {
                    return RetType{ std::unexpected(RangeCheck::lengthUnderflow) };
                }
            }
            if constexpr (hasMaximumLength<Type>)
            {
                if (value().size() > Type::MaximumLength)
                {
                    return RetType{ std::unexpected(RangeCheck::lengthOverflow) };
                }
            }
            if constexpr (hasPattern<Type>)
            {
                if (!matchesGlob(value(), Type::Pattern))
                {
                    return RetType{ std::unexpected(RangeCheck::patternMismatch) };
                }
            }
            return RetType{ value };
        }
    }
}// namespace DataLayer::Detail
