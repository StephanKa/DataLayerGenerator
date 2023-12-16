#pragma once

#include <cstdint>
#include <detail.h>
#ifdef USE_FILE_PERSISTENCE
#include <serialization.h>
#endif
#include <tuple>

namespace DataLayer {

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
    [[nodiscard]] Detail::CheckResult setDatapoint(uint32_t dataPointId, const T &value) const
    {
        return std::apply(
          [&](auto &...args) {
              bool ret = false;
              auto check{ Detail::RangeCheck::notChecked };

              (((dataPointId == args.getId()) && (setter(value, args, ret, check))) || ... || false);

              return Detail::CheckResult{ ret, check };
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
    constexpr static bool setter([[maybe_unused]] const auto &value, [[maybe_unused]] auto &args, [[maybe_unused]] bool &ret, Detail::RangeCheck &check)
    {
        if constexpr (Helper::WriteConcept<
                        std::remove_cvref_t<decltype(args.TypeAccess)>> && (std::is_same_v<std::remove_cvref_t<decltype(args())>, std::remove_cvref_t<decltype(value)>>)) {
            check = args.set(value);
            ret = true;
        }
        return true;
    }

    constexpr static bool getter([[maybe_unused]] auto &value, [[maybe_unused]] const auto &args, [[maybe_unused]] bool &ret)
    {
        if constexpr (Helper::ReadConcept<
                        std::remove_cvref_t<decltype(args.TypeAccess)>> && std::is_same_v<std::remove_cvref_t<decltype(args())>, std::remove_cvref_t<decltype(value)>>) {
            value = args();
            ret = true;
        }
        return true;
    }
};
}// namespace DataLayer
