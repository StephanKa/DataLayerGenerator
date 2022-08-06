#pragma once
#include <cstdint>
#include <fmt/format.h>
#include <helper.h>
#include <type_traits>

namespace DataLayer {
enum class Persistance : uint32_t { None, Cyclic, OnWrite };


// group definitions
template<uint16_t BaseId, Persistance persistence = Persistance::None, FixedString Name = "", auto Version = Version<0, 0, 0>{}> struct GroupInfo
{
    constexpr static Persistance persist{ persistence };
    constexpr static uint16_t baseId{ BaseId };
    constexpr static auto version{ Version };
    static constexpr char const *name = Name;
};

template<typename GroupInfo, typename... Datapoints> struct GroupDataPointMapping
{
    using ArgsT = std::tuple<Datapoints &...>;
    const ArgsT datapoints;
    const GroupInfo &group;
    constexpr explicit GroupDataPointMapping(GroupInfo &inputGroup, Datapoints &...dps) : datapoints(dps...), group(inputGroup) {}

    void printDatapoints() const
    {
        fmt::print("Group: {}\n", group.name);
        std::apply([&](const auto &...args) { ((fmt::print("{}: {}\n", args.name, args.getId())), ...); }, datapoints);
    }

    template<typename T> [[nodiscard]] bool setDatapoint(uint32_t dataPointId, const T &value) const
    {
        return std::apply(
          [&](auto &...args) {
              bool ret{};

              (((dataPointId == args.getId()) && (args.set(value), ret = true, true)) || ... || (false));

              return ret;
          },
          datapoints);
    }

    template<typename T> [[nodiscard]] bool getDatapoint(uint32_t dataPointId, T &value) const
    {
        return std::apply(
          [&](const auto &...args) {
              bool ret{};

              (((dataPointId == args.getId()) && (value = args(), ret = true, true)) || ... || (false));

              return ret;
          },
          datapoints);
    }
};

// data point definition
template<typename T, GroupInfo group, uint16_t id, typename Access, auto Version = Version<0, 0, 0>{}, FixedString Name = ""> class DataPoint
{
  public:
    DataPoint() = default;
    static constexpr char const *name = Name;

    constexpr static uint16_t getId() { return group.baseId + id; }
    constexpr static auto getVersion() { return Version; }

    template<typename A = Access>
        requires Helper::ReadConcept<A>
    T operator()() const
    {
        return m_value;
    }

    template<typename A = Access>
        requires Helper::ReadConcept<A>
    T &get()
    {
        // serialize can be done with free function "toBytes()"
        return m_value;
    }

    template<typename A = Access>
        requires Helper::WriteConcept<A>
    void set(const T &value)
    {
        // deserialize can be done with free function "fromBytes()"
        m_value = value;
    }

  private:
    T m_value{};
};

}// namespace DataLayer
