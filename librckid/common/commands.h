#pragma once

#include "tinytime.h"

/**
 
    - set brightness
    - rgb 
    - rumbler
    - audio on/off (?)
    - set time
    - set alarms
    - power off
    - reset
    - enter bootloader RP
    - enter bootloader AVR


 */
namespace rckid::cmd {

#define COMMAND(MSG_ID, NAME, ...)                               \
    class NAME  {                                                \
    public:                                                      \
        static uint8_t constexpr ID = MSG_ID;                    \
        uint8_t const id = MSG_ID;                               \
        static NAME const & fromBuffer(uint8_t const * buffer) { \
            return * reinterpret_cast<NAME const *>(buffer);     \
        }                                                        \
        __VA_ARGS__                                              \
    } __attribute__((packed))

    COMMAND(0, Nop);
    COMMAND(1, PowerOff);
    COMMAND(2, ResetRP);
    COMMAND(3, ResetAVR);
    COMMAND(4, BootloaderRP);
    COMMAND(5, BootloaderAVR);
    
    COMMAND(6, DebugModeOn);
    COMMAND(7, DebugModeOff);
    COMMAND(8, AudioEnabled);
    COMMAND(9, AudioDisabled);

    COMMAND(10, SetBrightness,
        uint8_t value;
        SetBrightness(uint8_t value): value{value} {}
    );

    COMMAND(11, SetTime, 
        TinyDate value;
        SetTime(TinyDate value): value{value} {}
    );

    COMMAND(40, RumblerOk);
    COMMAND(41, RumblerFail);

    COMMAND(42, Rumbler,
        uint8_t intensity;
        uint16_t duration; // duration in 10ms intervals
        Rumbler(uint8_t intensity, uint16_t duration): intensity{intensity}, duration{duration} {}
    );


/*
    COMMAND(100, RGBOn);
    COMMAND(101, RGBOff);

    COMMAND(102, RGBColor, 
        platform::Color color;
        RGBColor(platform::Color color): color{color} {}
    );

*/






} // namespace rckid::cmd