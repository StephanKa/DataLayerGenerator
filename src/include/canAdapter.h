#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <dispatcher.h>
#include <span>
#include <type_traits>

namespace DataLayer::Can
{
    inline constexpr size_t StandardPayloadSize = 8;
    inline constexpr size_t MaximumPayloadSize = 64;

    enum class Result : uint8_t
    {
        ok,
        sendFailed,
        payloadTooLarge,
        invalidPayload,
        rejected
    };

    struct Frame
    {
        uint32_t identifier{};
        uint8_t length{};
        std::array<std::byte, MaximumPayloadSize> payload{};
    };

    class Bus
    {
      public:
        virtual ~Bus() = default;
        virtual bool send(const Frame &frame) noexcept = 0;
    };

    template<typename DispatcherType, size_t PayloadCapacity = StandardPayloadSize>
    class Adapter
    {
        static_assert(PayloadCapacity > 0 && PayloadCapacity <= MaximumPayloadSize);

      public:
        constexpr Adapter(Bus &bus, DispatcherType &dispatcher) noexcept : m_bus(bus), m_dispatcher(dispatcher)
        {}

        template<typename T>
        [[nodiscard]] Result publish(uint32_t identifier, const T &value) noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>, "CAN transport requires a trivially copyable value");
            if constexpr (sizeof(T) > PayloadCapacity)
            {
                return Result::payloadTooLarge;
            }

            Frame frame{ .identifier = identifier, .length = static_cast<uint8_t>(sizeof(T)) };
            std::memcpy(frame.payload.data(), &value, sizeof(T));
            return m_bus.send(frame) ? Result::ok : Result::sendFailed;
        }

        template<typename T>
        [[nodiscard]] Result applySet(const Frame &frame) const noexcept
        {
            static_assert(std::is_trivially_copyable_v<T>, "CAN transport requires a trivially copyable value");
            if (frame.length != sizeof(T) || frame.length > PayloadCapacity)
            {
                return Result::invalidPayload;
            }

            T value{};
            std::memcpy(&value, frame.payload.data(), sizeof(T));
            return m_dispatcher.setDatapoint(frame.identifier, value).success ? Result::ok : Result::rejected;
        }

      private:
        Bus &m_bus;
        DispatcherType &m_dispatcher;
    };
}// namespace DataLayer::Can
