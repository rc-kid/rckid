#pragma once

#include <rckid/device.h>
#include <platform/tinydate.h>
#include <config.h>

namespace rckid {

    class TransferrableState {
    public:

        DeviceState state;
        uint32_t wakeupReason = 0;
        uint32_t wakeupCounter = 0;
        TinyDateTime time;
        uint32_t uptime = 0;
        int16_t temp = 0;
        uint32_t const version = RCKID_AVR_FIRMWARE_VERSION;
        bool debugUart = true;
        uint8_t storage[1024];
    } __attribute__((packed));

    constexpr uint32_t TS_SIZE = sizeof(TransferrableState) - 1024;


} // namespace rckid

namespace rckid::cmd {
    #define COMMAND(MSG_ID, NAME, ...)                               \
        PACKED(class NAME  {                                         \
        public:                                                      \
            static uint8_t constexpr ID = MSG_ID;                    \
            uint8_t const id = MSG_ID;                               \
            static NAME const & fromBuffer(uint8_t const * buffer) { \
                return * reinterpret_cast<NAME const *>(buffer);     \
            }                                                        \
            __VA_ARGS__                                              \
        });                                                          \
        static_assert(sizeof(NAME) <= 131);              

    #include "avr-commands.inc"

    inline uint8_t getCommandSize(uint8_t id) {
    #define COMMAND(MSG_ID, NAME, ...)                               \
            case MSG_ID:                                             \
                return sizeof(NAME);                                 \

        switch (id) {
        #include "avr-commands.inc"
            default:
                return 0;
        }
    }

} // namespace rckid::cmd