#pragma once
#include <algorithm>
#include <array>
#include <compare>
#include <span>
#include <string_view>

namespace DataLayer
{
    enum class PersistenceType : uint8_t
    {
        None,
        Cyclic,
        OnWrite
    };

    struct Version
    {
        constexpr Version(uint32_t Major, uint32_t Minor, uint32_t Build) : major(Major), minor(Minor), build(Build)
        {}

        constexpr Version() = default;

        uint32_t major{};
        uint32_t minor{};
        uint32_t build{};

        constexpr auto operator<=>(const Version &) const = default;
        constexpr bool operator==(const Version &) const = default;
    };
}// namespace DataLayer

template<size_t N>
struct FixedString
{
    std::array<char, N + 1> buf{};

    consteval FixedString(const char (&str)[N + 1])
    {
        std::copy_n(str, N + 1, buf.begin());
    }

    [[nodiscard]] constexpr operator std::string_view() const
    {
        return { buf.data(), N };
    }

    [[nodiscard]] static constexpr size_t size() noexcept
    {
        return N;
    }
};

template<unsigned N>
FixedString(const char (&)[N]) -> FixedString<N - 1>;

namespace Helper
{
    // helper classes
    struct READ_ONLY
    {
    };

    struct WRITE_ONLY
    {
    };

    struct READ_WRITE
      : READ_ONLY
      , WRITE_ONLY
    {
    };

    // helper concepts
    template<typename Access>
    concept WriteConcept = std::is_same_v<Access, READ_WRITE> || std::is_same_v<Access, WRITE_ONLY>;

    template<typename Access>
    concept ReadConcept = std::is_same_v<Access, READ_WRITE> || std::is_same_v<Access, READ_ONLY>;

    // helper type for the visitor #4
    template<class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
}// namespace Helper
