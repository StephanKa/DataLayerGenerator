#pragma once
#ifdef USE_TRANSACTIONAL_UPDATES
#include <array>
#endif
#include <cstddef>
#include <cstdint>
#include <detail.h>
#ifdef USE_FILE_PERSISTENCE
#include <serialization.h>
#endif
#include <tuple>

namespace DataLayer
{
#ifdef USE_TRANSACTIONAL_UPDATES
    template<typename T>
    struct DatapointUpdate
    {
        uint32_t dataPointId;
        const T &value;
    };

    template<typename T>
    DatapointUpdate(uint32_t, const T &) -> DatapointUpdate<T>;
#endif

    template<typename Mapping>
    struct DataPointSize;

    template<typename GroupInfo, typename... Datapoints>
    struct GroupDataPointMapping
    {
        using ArgsT = std::tuple<Datapoints &...>;
        ArgsT datapoints;
        GroupInfo &group;

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
        [[nodiscard]] constexpr Detail::CheckResult setDatapoint(uint32_t dataPointId, const T &value) const
        {
            return std::apply(
              [&](auto &...args) {
                  bool ret = false;
                  auto check{ Detail::RangeCheck::notChecked };

                  ((args.matchesId(dataPointId) && (setter(value, args, ret, check))) || ... || false);

                  return Detail::CheckResult{ .success = ret, .check = check };
              },
              datapoints);
        }

        template<typename T>
        [[nodiscard]] constexpr bool getDatapoint(uint32_t dataPointId, T &value) const
        {
            return std::apply(
              [&](const auto &...args) {
                  bool ret = false;

                  ((args.matchesId(dataPointId) && (getter(value, args, ret))) || ... || false);

                  return ret;
              },
              datapoints);
        }

#ifdef USE_TRANSACTIONAL_UPDATES

        template<typename... Updates>
            requires(sizeof...(Updates) > 0)
        [[nodiscard]] constexpr Detail::CheckResult setDatapointsAtomically(const Updates &...updates) const
        {
            const std::array dataPointIds{ updates.dataPointId... };
            for (size_t index = 0; index < dataPointIds.size(); ++index)
            {
                for (size_t comparison = index + 1; comparison < dataPointIds.size(); ++comparison)
                {
                    if (dataPointIds[index] == dataPointIds[comparison])
                    {
                        return { .success = false, .check = Detail::RangeCheck::notChecked };
                    }
                }
            }

            auto check = Detail::RangeCheck::notChecked;
            if (!(validateUpdate(updates, check) && ...))
            {
                return { .success = false, .check = check };
            }
            (commitUpdate(updates), ...);
            return { .success = true, .check = Detail::RangeCheck::ok };
        }

#endif

#ifdef USE_FILE_PERSISTENCE

        [[nodiscard]] SerializationStatus serializeGroup(const std::filesystem::path &path) const
        {
            Serialization value(group.version, group.baseId, path, datapoints);
            return value.write();
        }

        [[nodiscard]] SerializationStatus deserializeGroup(const std::filesystem::path &path) const
        {
            Deserialization value(group.version, group.baseId, path, datapoints, group.allowUpgrade);
            return value.read();
        }

        template<DataLayer::Persistence::StorageBackend Backend>
        [[nodiscard]] SerializationStatus serializeGroup(Backend &backend) const
        {
            return DataLayer::Persistence::serialize(backend, group.version, group.baseId, datapoints);
        }

        template<DataLayer::Persistence::StorageBackend Backend>
        [[nodiscard]] SerializationStatus deserializeGroup(const Backend &backend) const
        {
            return DataLayer::Persistence::deserialize(backend, group.version, group.baseId, datapoints, group.allowUpgrade);
        }

#endif

      private:
#ifdef USE_TRANSACTIONAL_UPDATES
        constexpr bool validateUpdate(const auto &update, Detail::RangeCheck &check) const
        {
            bool found = false;
            bool valid = false;
            std::apply(
              [&](const auto &...args) {
                  (([&] {
                      if (!found && args.matchesId(update.dataPointId))
                      {
                          found = true;
                          valid = validateValue(update, args, check);
                      }
                  }()),
                    ...);
              },
              datapoints);
            if (!found)
            {
                check = Detail::RangeCheck::notChecked;
            }
            return found && valid;
        }

        constexpr void commitUpdate(const auto &update) const
        {
            std::apply([&](auto &...args) { ((args.matchesId(update.dataPointId) ? commitValue(update, args) : void()), ...); }, datapoints);
        }

        constexpr static bool validateValue(const auto &update, const auto &args, Detail::RangeCheck &check)
        {
            if constexpr (Helper::WriteConcept<std::remove_cvref_t<decltype(args.TypeAccess)>>
                          && std::is_same_v<std::remove_cvref_t<decltype(args())>, std::remove_cvref_t<decltype(update.value)>>)
            {
                const auto value = Detail::checkValue(update.value);
                check = value.has_value() ? Detail::RangeCheck::ok : value.error();
                return value.has_value();
            }
            check = Detail::RangeCheck::notChecked;
            return false;
        }

        constexpr static void commitValue(const auto &update, auto &args)
        {
            if constexpr (Helper::WriteConcept<std::remove_cvref_t<decltype(args.TypeAccess)>>
                          && std::is_same_v<std::remove_cvref_t<decltype(args())>, std::remove_cvref_t<decltype(update.value)>>)
            {
                std::ignore = args.set(update.value);
            }
        }
#endif

        constexpr static bool setter([[maybe_unused]] const auto &value, [[maybe_unused]] auto &args, [[maybe_unused]] bool &ret, [[maybe_unused]] Detail::RangeCheck &check)
        {
            if constexpr (Helper::WriteConcept<std::remove_cvref_t<decltype(args.TypeAccess)>>
                          && (std::is_same_v<std::remove_cvref_t<decltype(args())>, std::remove_cvref_t<decltype(value)>>))
            {
                check = args.set(value);
                ret = true;
            }
            return true;
        }

        constexpr static bool getter([[maybe_unused]] auto &value, [[maybe_unused]] const auto &args, [[maybe_unused]] bool &ret)
        {
            if constexpr (Helper::ReadConcept<std::remove_cvref_t<decltype(args.TypeAccess)>>
                          && std::is_same_v<std::remove_cvref_t<decltype(args())>, std::remove_cvref_t<decltype(value)>>)
            {
                value = args();
                ret = true;
            }
            return true;
        }
    };

    template<typename GroupInfo, typename... Datapoints>
    struct DataPointSize<GroupDataPointMapping<GroupInfo, Datapoints...>>
    {
        static constexpr std::size_t objectBytes = (sizeof(Datapoints) + ... + 0U);
        static constexpr std::size_t payloadBytes = (sizeof(typename Datapoints::ValueType) + ... + 0U);
    };
}// namespace DataLayer
