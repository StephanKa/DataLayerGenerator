#pragma once
#include <cstdint>
#include <tuple>
#ifdef USE_FMT
#include <fmt/format.h>
#endif
#include <helper.h>
#include <type_traits>

namespace DataLayer {
enum class Persistance : uint32_t { None, Cyclic, OnWrite };

// group definitions
template<uint16_t BaseId, FixedString Name, Persistance persistence = Persistance::None, auto Version = Version<0, 0, 0>{}> struct GroupInfo
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
#ifdef USE_FMT
        fmt::print("Group: {}\n", group.name);
        std::apply([&](const auto &...args) { ((fmt::print("{}: {:#06x}\n", args.name, args.getId())), ...); }, datapoints);
#endif
    }

    template<typename T> [[nodiscard]] bool setDatapoint(uint32_t dataPointId, const T &value) const
    {

        return std::apply(
          [&](auto &...args) {
              bool ret = false;

              (((dataPointId == args.getId()) && (setter(value, args, ret))) || ... || false);

              return ret;
          },
          datapoints);
    }

    template<typename T> [[nodiscard]] bool getDatapoint(uint32_t dataPointId, T &value) const
    {
        return std::apply(
          [&](const auto &...args) {
              bool ret = false;

              (((dataPointId == args.getId()) && (getter(value, args, ret))) || ... || false);

              return ret;
          },
          datapoints);
    }

  private:
    constexpr static bool setter([[maybe_unused]] const auto &value, [[maybe_unused]] auto &args, [[maybe_unused]] bool &ret)
    {
        if constexpr (Helper::WriteConcept<std::remove_cvref_t<decltype(args.TypeAccess)>>) {
            args.set(value);
            ret = true;
        }
        return true;
    }

    constexpr static bool getter([[maybe_unused]] auto &value, [[maybe_unused]] const auto &args, [[maybe_unused]] bool &ret)
    {
        if constexpr (Helper::ReadConcept<std::remove_cvref_t<decltype(args.TypeAccess)>>) {
            value = args();
            ret = true;
        }
        return true;
    }
};

// data point definition
template<typename T, GroupInfo group, uint16_t id, typename Access, auto Version = Version<0, 0, 0>{}, FixedString Name = ""> class DataPoint
{
  public:
    constexpr static Access TypeAccess{};
    constexpr DataPoint() = default;
    constexpr explicit DataPoint(T value) : m_value(value) {}
    static constexpr char const *name = Name;

    constexpr static uint16_t getId() { return group.baseId + id; }
    constexpr static auto getVersion() { return Version; }

    template<typename A = Access>
    requires Helper::ReadConcept<A> T operator()() const { return m_value; }

    template<typename A = Access>
    requires Helper::ReadConcept<A> T &get()
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
