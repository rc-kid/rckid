#pragma once

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
        static_assert(sizeof(NAME) <= 32)              

    COMMAND(0, Nop);
    COMMAND(1, PowerOff);
    COMMAND(2, Sleep);
    COMMAND(3, ResetRP);
    COMMAND(4, BootloaderRP);
    COMMAND(5, ResetAVR);
    COMMAND(6, BootloaderAVR);
    COMMAND(7, DebugModeOn);
    COMMAND(8, DebugModeOff);
    COMMAND(9, SetBrightness,
        uint8_t value;
        SetBrightness(uint8_t value): value{value} {}
    );
    COMMAND(10, SetTime, 
        TinyDate value;
        SetTime(TinyDate value): value{value} {}
    );
    /** Sets the alarm to given value and enables it. 
     */
    COMMAND(11, SetAlarm, 
        TinyAlarm value;
        SetAlarm(TinyAlarm value): value{value} {}
    );
    /** Clears the alarm flag in the status and clears the alarm itself. If the alarm should happen again, SetAlarm command must be issued after clearing the alarm.
     */
    COMMAND(12, ClearAlarm);


} // namespace rckid::cmd