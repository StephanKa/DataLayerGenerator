#pragma once
#include <cstdint>
#include <tuple>
#include <variant>
#ifdef USE_FMT
#include <fmt/format.h>
#endif
#include <dataPointGroupMapping.h>
#include <datapoint.h>
#include <detail.h>
#include <groupInfo.h>
#include <helper.h>
#include <type_traits>
#ifdef USE_FILE_PERSISTENCE
#include <serialization.h>
#endif

namespace DataLayer {

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
              auto check{ Detail::RangeCheck::notChecked };
              ((setter(dataPointId, value, args, ret, check)) || ... || false);
              return Detail::CheckResult{ ret, check };
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
    constexpr static bool setter(const uint32_t dataPointId, const auto &value, [[maybe_unused]] auto &args, [[maybe_unused]] bool &ret, Detail::RangeCheck &check)
    {
        const auto returnCheck = args.setDatapoint(dataPointId, value);
        check = returnCheck.check;
        ret |= returnCheck.success;
        return !ret;
    }

    constexpr static bool getter(const uint32_t dataPointId, auto &value, [[maybe_unused]] const auto &args, [[maybe_unused]] bool &ret)
    {
        ret |= args.getDatapoint(dataPointId, value);
        return !ret;
    }
};

}// namespace DataLayer
