#pragma once

#include "platform.h"

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
    } __attribute__((packed));                                   \
    static_assert(sizeof(NAME) <= 32)              

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
        platform::TinyDate value;
        SetTime(platform::TinyDate value): value{value} {}
    );

    COMMAND(40, Rumbler,
        RumblerEffect effect;
        Rumbler(RumblerEffect effect): effect{effect} {}
    );

    /** Turns all LEDs off immediately. 
     */
    COMMAND(100, RGBOff);

    /** Specifies the RGB effect for a particular LED. 
    */
    COMMAND(101, SetRGBEffect, 
        uint8_t index;
        RGBEffect effect;
        SetRGBEffect(uint8_t index, RGBEffect const & effect): index{index}, effect{effect} {}
    );

    COMMAND(102, SetRGBEffects, 
        RGBEffect a;
        RGBEffect b;
        RGBEffect dpad;
        RGBEffect sel;
        RGBEffect start;
        SetRGBEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start):
            a{a}, b{b}, dpad{dpad}, sel{sel}, start{start} {}
    );

} // namespace rckid::cmd