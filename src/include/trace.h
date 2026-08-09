#pragma once

#include <cstdint>

namespace DataLayer::Trace
{
    enum class EventKind : uint8_t
    {
        WriteRejected,
        RangeRejected,
        PersistenceError,
        Transport
    };

    enum class Source : uint8_t
    {
        Core,
        Persistence,
        Can,
        Mqtt
    };

    struct Record
    {
        EventKind kind;
        Source source;
        uint32_t identifier;
        uint32_t detail;
        bool success;
    };

#ifdef USE_TRACE_HOOKS
#ifndef DATALAYER_TRACE_HOOK
#define DATALAYER_TRACE_HOOK(record) static_cast<void>(record)
#endif

    inline void emit(const Record &record) noexcept
    {
        DATALAYER_TRACE_HOOK(record);
    }
#endif
}// namespace DataLayer::Trace
