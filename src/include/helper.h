#pragma once
#include <array>
#include <cstring>

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

}// namespace Helper
