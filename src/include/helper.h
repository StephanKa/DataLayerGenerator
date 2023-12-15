#pragma once

#include <array>
#include <cstring>

namespace DataLayer {

enum class Persistance : uint8_t
{
    None,
    Cyclic,
    OnWrite
};
}

struct Version
{
    constexpr Version(uint32_t Major, uint32_t Minor, uint32_t Build) : major(Major), minor(Minor), build(Build)
    {}

    constexpr Version() = default;

    const uint32_t major{};
    const uint32_t minor{};
    const uint32_t build{};

    constexpr bool operator!=(const Version &rhs) const
    {
        return rhs.major != major || rhs.minor != minor || rhs.build != build;
    }

    constexpr bool operator>(const Version &rhs) const
    {
        return rhs.major > major || rhs.minor > minor || rhs.build > build;
    }
};

template<unsigned N>
struct FixedString
{
    std::array<char, N + 1> buf{};

    consteval FixedString(char const *input)
    {
        for (unsigned i = 0; i != N; ++i) {
            buf[i] = input[i];
        }
    }

    [[nodiscard]] constexpr operator char const *() const
    {
        return buf.data();
    }
};

template<unsigned N>
FixedString(const char (&)[N]) -> FixedString<N - 1>;

namespace Helper {
// helper classes
struct READONLY
{
};

struct WRITEONLY
{
};

struct READWRITE
  : public READONLY
  , public WRITEONLY
{
};

// helper concepts
template<typename Access>
concept WriteConcept = std::is_same_v<Access, READWRITE> || std::is_same_v<Access, WRITEONLY>;

template<typename Access>
concept ReadConcept = std::is_same_v<Access, READWRITE> || std::is_same_v<Access, READONLY>;


template<template<typename...> class BaseTemplate, typename Derived, typename TCheck = void>
struct testBaseTemplate;

template<template<typename...> class BaseTemplate, typename Derived>
using is_base_template_of = typename testBaseTemplate<BaseTemplate, Derived>::is_base;

// Derive - is a class. Let inherit from Derive, so it can cast to its protected parents
template<template<typename...> class BaseTemplate, typename Derived>
struct testBaseTemplate<BaseTemplate, Derived, std::enable_if_t<std::is_class_v<Derived>>> : Derived
{
    template<typename... T>
    static constexpr std::true_type test(BaseTemplate<T...> *);

    static constexpr std::false_type test(...);

    using is_base = decltype(test(static_cast<testBaseTemplate *>(nullptr)));
};

// Derive - is not a class, so it is always false_type
template<template<typename...> class BaseTemplate, typename Derived>
struct testBaseTemplate<BaseTemplate, Derived, std::enable_if_t<!std::is_class_v<Derived>>>
{
    using is_base = std::false_type;
};

// helper type for the visitor #4
template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;


}// namespace Helper
