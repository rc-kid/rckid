#pragma once

#include <rckid/device.h>
#include <platform/tinydate.h>

namespace rckid {

    class TransferrableState {
    public:
        DeviceState state;
        TinyDateTime time;
        uint8_t storage[1024];
    } __attribute__((packed));


} // namespace rckid