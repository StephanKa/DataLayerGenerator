#pragma once
#include <array>
#include <cstring>

template<uint32_t Major, uint32_t Minor, uint32_t Build>
struct Version
{
    consteval Version() = default;
    constexpr static uint32_t major{ Major };
    constexpr static uint32_t minor{ Minor };
    constexpr static uint32_t build{ Build };
};

template<unsigned N>
struct FixedString
{
    std::array<char, N + 1> buf{};
    consteval FixedString(char const *input)
    {
        for (unsigned i = 0; i != N; ++i) { buf[i] = input[i]; }
    }
    constexpr operator char const *() const
    {
        return buf.data();
    }
};
template<unsigned N>
FixedString(const char (&)[N]) -> FixedString<N - 1>;

namespace Helper {
// helper functions
template<typename T>
constexpr auto bytesTo(const uint8_t *const data)
{
    T tmp;
    memcpy(&tmp, data, sizeof(T));
    return tmp;
}

template<typename T>
constexpr auto toBytes(const T &input, uint8_t *const daten)
{
    memcpy(daten, reinterpret_cast<const uint8_t *>(&input), sizeof(T));
    return sizeof(T);
}

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
