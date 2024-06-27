#pragma once

#include "common/errors.h"

#ifndef ASSERT
#define ASSERT(...)
#endif

#ifndef UNREACHABLE
#define UNREACHABLE
#endif

namespace rckid::radio {

    /** DeviceID
     
        The device ID identifies an RCKid's device for the transmissions. The device ID is not tied to the device forever.
     */
    using DeviceId = uint8_t;

    /** Device ID 0 is reserved as broadcast device.
     */
    static constexpr DeviceId BroadcastId = 0;

    enum class ConnectionKind : uint8_t {

    }; // radio::ConnectionKind

    enum class BroadcastKind : uint8_t {

    }; // radio::BroadcastKind

} // rckid::radio