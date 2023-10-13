#pragma once
#include <cstdint>
#include <tuple>
#ifdef USE_FMT
#include <fmt/format.h>
#endif
#include <helper.h>
#include <span>
#include <type_traits>
#ifdef USE_FILE_PERSISTENCE
#include <serialization.h>
#endif

namespace DataLayer {

namespace Detail {
    template<typename T, size_t N>
    [[nodiscard]] constexpr std::array<T, N> make_array(T value)
    {
        std::array<T, N> temp{};
        for (auto &val : temp) {
            val = value;
        }
        return temp;
    }

    template<typename Type>
    concept IsArray = requires(Type val)
    {
        val.begin();
    };
}// namespace Detail

enum class Persistance : uint32_t
{
    None,
    Cyclic,
    OnWrite
};

// group definitions
template<uint16_t BaseId, FixedString Name, bool AllowUpgrade, Persistance persistence = Persistance::None, auto Version = Version{ 0, 0, 0 }>
struct GroupInfo
{
    constexpr static Persistance persist{ persistence };
    constexpr static uint16_t baseId{ BaseId };
    constexpr static auto version{ Version };
    static constexpr char const *name = Name;
    constexpr static bool allowUpgrade = AllowUpgrade;
};

template<typename GroupInfo, typename... Datapoints>
struct GroupDataPointMapping
{
    using ArgsT = std::tuple<Datapoints &...>;
    ArgsT datapoints;
    const GroupInfo &group;
    consteval GroupDataPointMapping() = default;
    consteval explicit GroupDataPointMapping(GroupInfo &inputGroup, Datapoints &...dps) : datapoints(dps...), group(inputGroup)
    {}

    void printDatapoints() const
    {
#ifdef USE_FMT
        fmt::print("Group: {}\n", group.name);
        std::apply([&](const auto &...args) { ((fmt::print("{}: {:#06x}\n", args.name, args.getId())), ...); }, datapoints);
#endif
    }

    template<typename T>
    [[nodiscard]] bool setDatapoint(uint32_t dataPointId, const T &value) const
    {

        return std::apply(
          [&](auto &...args) {
              bool ret = false;

              (((dataPointId == args.getId()) && (setter(value, args, ret))) || ... || false);

              return ret;
          },
          datapoints);
    }

    template<typename T>
    [[nodiscard]] bool getDatapoint(uint32_t dataPointId, T &value) const
    {
        return std::apply(
          [&](const auto &...args) {
              bool ret = false;

              (((dataPointId == args.getId()) && (getter(value, args, ret))) || ... || false);

              return ret;
          },
          datapoints);
    }

#ifdef USE_FILE_PERSISTENCE
    [[nodiscard]] SerializationStatus serializeGroup(std::string_view path) const
    {
        Serialization value(group.version, path, datapoints);
        return value.write();
    }

    [[nodiscard]] SerializationStatus deserializeGroup(std::string_view path) const
    {
        Deserialization value(group.version, path, datapoints, group.allowUpgrade);
        return value.read();
    }
#endif

  private:
    [[nodiscard]] constexpr static bool setter([[maybe_unused]] const auto &value, [[maybe_unused]] auto &args, [[maybe_unused]] bool &ret)
    {
        if constexpr (Helper::WriteConcept<
                        std::remove_cvref_t<decltype(args.TypeAccess)>> && (std::is_same_v<std::remove_cvref_t<decltype(args())>, std::remove_cvref_t<decltype(value)>>)) {
            args.set(value);
            ret = true;
        }
        return true;
    }

    [[nodiscard]] constexpr static bool getter([[maybe_unused]] auto &value, [[maybe_unused]] const auto &args, [[maybe_unused]] bool &ret)
    {
        if constexpr (Helper::ReadConcept<
                        std::remove_cvref_t<decltype(args.TypeAccess)>> && std::is_same_v<std::remove_cvref_t<decltype(args())>, std::remove_cvref_t<decltype(value)>>) {
            value = args();
            ret = true;
        }
        return true;
    }
};

// data point definition
template<typename T, GroupInfo group, uint16_t id, typename Access, auto Version = Version{ 0, 0, 0 }, FixedString Name = "", bool AllowUpgrade = false>
class DataPoint
{
  public:
    constexpr static Access TypeAccess{};
    consteval DataPoint() = default;

    consteval explicit DataPoint(T value) : m_value(value)
    {}

    static constexpr char const *name = Name;

    [[nodiscard]] constexpr static uint16_t getId()
    {
        return group.baseId + id;
    }
    [[nodiscard]] constexpr static auto getVersion()
    {
        return Version;
    }

    // function to read everytime
    [[nodiscard]] constexpr T operator()() const
    {
        return m_value;
    }

    // function to write anyway
    [[nodiscard]] constexpr DataPoint &operator=(const T &value)
    {
        m_value = value;
        return *this;
    }

    // function that will be restricted by READ and READWRITE access
    template<typename A = Access>
    requires Helper::ReadConcept<A> T &get()
    {
        return m_value;
    }

    // function that will be restricted by READ and READWRITE access
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
        return std::as_bytes(std::span<const T, 1>{ std::addressof(m_value), 1 });
    }

    // function that will be restricted by WRITE and READWRITE access
    template<typename A = Access>
    requires Helper::WriteConcept<A>
    constexpr void set(const T &value)
    {
        m_value = value;
    }

    // function that will be restricted by WRITE and READWRITE access
    template<typename A = Access>
    requires Helper::WriteConcept<A> && Detail::IsArray<T>
    constexpr void set(size_t index, const auto &value)
    {
        m_value.at(index) = value;
    }

    template<typename A = Access>
    requires Helper::WriteConcept<A>
    void deserialize(const auto &value)
    {
        std::memcpy(&m_value, value.data(), sizeof(T));
    }

    template<typename Type = T>
    requires Detail::IsArray<Type>
    [[nodiscard]] constexpr auto size()
    {
        return m_value.size();
    }

    constexpr bool getIsUpgradeAllowed()
    {
        return AllowUpgrade;
    }

  private:
    T m_value{};
};

template<typename... GroupInfos>
struct Dispatcher
{
    using ArgsT = std::tuple<GroupInfos &...>;
    ArgsT groups;
    consteval explicit Dispatcher(GroupInfos &...groupInfos) : groups(groupInfos...)
    {}

    void printStructure() const
    {
#ifdef USE_FMT
        fmt::print("Structure:\n");
        std::apply([&](const auto &...args) { ((args.printDatapoints()), ...); }, groups);
#endif
    }

    template<typename T>
    [[nodiscard]] bool setDatapoint(uint32_t dataPointId, const T &value) const
    {

        return std::apply(
          [&](auto &...args) {
              bool ret = false;

              ((setter(dataPointId, value, args, ret)) || ... || false);

              return ret;
          },
          groups);
    }

    template<typename T>
    [[nodiscard]] bool getDatapoint(uint32_t dataPointId, T &value) const
    {
        return std::apply(
          [&](const auto &...args) {
              bool ret = false;

              ((getter(dataPointId, value, args, ret)) || ... || false);

              return ret;
          },
          groups);
    }

  private:
    [[nodiscard]] constexpr static bool setter(const uint32_t dataPointId, const auto &value, [[maybe_unused]] auto &args, [[maybe_unused]] bool &ret)
    {
        ret |= args.setDatapoint(dataPointId, value);
        return !ret;
    }

    [[nodiscard]] constexpr static bool getter(const uint32_t dataPointId, auto &value, [[maybe_unused]] const auto &args, [[maybe_unused]] bool &ret)
    {
        ret |= args.getDatapoint(dataPointId, value);
        return !ret;
    }
};

}// namespace DataLayer
