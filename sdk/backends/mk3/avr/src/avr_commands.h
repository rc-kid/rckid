#pragma once

#include <platform/tinydate.h>

#include <rckid/effects.h>

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
    



    COMMAND(40, ReadFlashPage,
        uint16_t page;
        ReadFlashPage(uint16_t page): page{page} {}
    );

    COMMAND(41, ReadEEPROMPage,
        uint16_t page;
        ReadEEPROMPage(uint16_t page): page{page} {}
    );

    COMMAND(42, ReadRAMPage,
        uint16_t page;
        ReadRAMPage(uint16_t page): page{page} {}
    );

    COMMAND(43, WriteFlashPage,
        uint16_t page;
        uint8_t data[128];
        WriteFlashPage(uint16_t page, int8_t const * data): page{page} {
            memcpy(this->data, data, sizeof(data));
        }
    );

    COMMAND(44, WriteRAMPage,
        uint16_t page;
        uint8_t data[32];
        WriteRAMPage(uint16_t page, uint8_t const * data): page{page} {
            memcpy(this->data, data, sizeof(data));
        }
    );

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

    COMMAND(103, Rumbler,
        RumblerEffect effect;
        Rumbler(RumblerEffect const & effect): effect{effect} {}
    );


    COMMAND(150, SetNotification,
        RGBEffect effect;
        SetNotification(RGBEffect const & effect): effect{effect} {}
    );
    COMMAND(151, ChargerConnected);
    COMMAND(152, ChargerDisconnected);
    COMMAND(153, ChargerDone);
    COMMAND(154, ChargerError);







} // namespace rckid::cmd