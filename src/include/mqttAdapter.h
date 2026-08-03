#pragma once

#include <cstddef>
#include <cstring>
#include <dispatcher.h>
#include <span>
#include <string_view>
#include <type_traits>

namespace DataLayer::Mqtt
{
    enum class Result : uint8_t
    {
        ok,
        publishFailed,
        invalidPayload,
        rejected
    };

    class Client
    {
      public:
        virtual ~Client() = default;
        virtual bool publish(std::string_view topic, std::span<const std::byte> payload, bool retain) noexcept = 0;
    };

    template<typename DispatcherType>
    class Adapter
    {
      public:
        constexpr Adapter(Client &client, DispatcherType &dispatcher) noexcept : m_client(client), m_dispatcher(dispatcher)
        {}

        template<typename T>
        [[nodiscard]] Result publish(std::string_view topic, const T &value, bool retain = false) noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>, "MQTT binary transport requires a trivially copyable value");
            const auto payload = std::as_bytes(std::span{ &value, 1 });
            return m_client.publish(topic, payload, retain) ? Result::ok : Result::publishFailed;
        }

        template<typename T>
        [[nodiscard]] Result applySet(uint32_t dataPointId, std::span<const std::byte> payload) const noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>, "MQTT binary transport requires a trivially copyable value");
            if (payload.size() != sizeof(T))
            {
                return Result::invalidPayload;
            }

            T value{};
            std::memcpy(&value, payload.data(), sizeof(T));
            return m_dispatcher.setDatapoint(dataPointId, value).success ? Result::ok : Result::rejected;
        }

      private:
        Client &m_client;
        DispatcherType &m_dispatcher;
    };
}// namespace DataLayer::Mqtt
