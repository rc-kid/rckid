#pragma once

#include <rckid/device.h>
#include <platform/tinydate.h>
#include <config.h>

namespace rckid {

    class TransferrableState {
    public:
        DeviceState state;
        TinyDateTime time;
        uint32_t uptime = 0;
        int16_t temp = 0;
        uint32_t const version = RCKID_AVR_FIRMWARE_VERSION;
        uint8_t storage[1024];
    } __attribute__((packed));

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
        static_assert(sizeof(NAME) <= 131)              

    /** No operation. 
     
        Also resets the read address to the beginning of the device status (default).
     */
    COMMAND(0, Nop);

    /** Returns the version of the AVR firmware.
     */
    COMMAND(1, GetVersion);
    
    /** Sets the read address for the next I2C read to the beginning of the time information.
     */
    COMMAND(2, GetTime);
    
    /** Sets the date & time kept by the AVR.
     */
    COMMAND(3, SetTime,
        TinyDateTime value;
        explicit SetTime(TinyDateTime value): value{value} {}
    );

    /** Sets the read address for the next I2C read to the given offset in the 1024 bytes of storage data. 
     */
    COMMAND(4, ReadStorage,
        uint16_t offset;
        explicit ReadStorage(uint16_t offset = 0): offset{offset} {}
    );

    /** Writes bytes to the storage buffer at given offset.
     */
    COMMAND(5, WriteStorage,
        uint16_t offset;
        uint8_t data[RCKID_I2C_MAX_ASYNC_MSG_SIZE - sizeof(offset) - 1];
        WriteStorage(uint16_t offset, uint8_t const * data, uint32_t len): offset{offset} {
            ASSERT(len <= sizeof(this->data));
            memcpy(this->data, data, len);
        }
    );

    // TODO actually implement the memory commands
    COMMAND(6, ReadEEPROM);
    COMMAND(7, WriteEEPROM);

    // power management commands

    /** Immediately powers off the device by cutting the 3V3 power supply for the cartridge and RP2350.
     */
    COMMAND(8, Poweroff);

    COMMAND(9, PowerOffAck);



} // namespace rckid::cmd