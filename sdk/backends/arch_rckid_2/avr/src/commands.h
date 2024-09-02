#pragma once

#include <platform.h>
#include <platform/tinydate.h>

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
    COMMAND(2, Sleep);
    COMMAND(4, ResetRP);
    COMMAND(5, ResetAVR);
    COMMAND(6, BootloaderRP);
    COMMAND(7, BootloaderAVR);
    
    COMMAND(8, DebugModeOn);
    COMMAND(9, DebugModeOff);
    COMMAND(10, AudioEnabled);
    COMMAND(11, AudioDisabled);

    COMMAND(12, SetBrightness,
        uint8_t value;
        SetBrightness(uint8_t value): value{value} {}
    );

    COMMAND(13, SetTime, 
        TinyDate value;
        SetTime(TinyDate value): value{value} {}
    );

    /** Instructs the RPI to enable reading from the display by the RP data lines by pulling the DISP_RDX line low. 
     */
    COMMAND(20, DisplayRead);

    /** Instructs the RPI to enable writing from the display by the RP data lines by letting the DISP_RDC lien float (pulled up externally). 
     */
    COMMAND(21, DisplayWrite);

    /** Clears the AVR last error information. 
     */
    COMMAND(30, ResetAVRError);

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