#pragma once
#include <platform/tinydate.h>

#include <rckid/effects.h>

#include "avr_state.h"

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
    /** Acknowledges the power off interrupt request from AVR. 
     
        This command is sent to inform AVR that RP2350 is in responsive state and is currently working towards shutting the device down. AVR thus increases the 
     */
    COMMAND(2, PowerOffAck);
    COMMAND(3, Sleep);
    COMMAND(4, ResetRP);
    COMMAND(5, BootloaderRP);
    COMMAND(6, ResetAVR);
    COMMAND(7, BootloaderAVR);
    COMMAND(8, DebugModeOn);
    COMMAND(9, DebugModeOff);
    
    COMMAND(27, ReadFlashPage,
        uint16_t page;
        ReadFlashPage(uint16_t page): page{page} {}
    );

    COMMAND(28, ReadEEPROMPage,
        uint16_t page;
        ReadEEPROMPage(uint16_t page): page{page} {}
    );

    COMMAND(29, ReadRAMPage,
        uint16_t page;
        ReadRAMPage(uint16_t page): page{page} {}
    );

    COMMAND(30, WriteFlashPage,
        uint16_t page;
        uint8_t data[128];
        WriteFlashPage(uint16_t page, int8_t const * data): page{page} {
            memcpy(this->data, data, sizeof(this->data));
        }
    );

    COMMAND(31, WriteRAMPage,
        uint16_t page;
        uint8_t data[32];
        WriteRAMPage(uint16_t page, uint8_t const * data): page{page} {
            memcpy(this->data, data, sizeof(this->data));
        }
    );

    COMMAND(32, SetNotification,
        RGBEffect effect;
        SetNotification(RGBEffect const & effect): effect{effect} {}
    );

    COMMAND(33, SetBrightness,
        uint8_t value;
        bool updateState;
        SetBrightness(uint8_t value, bool updateState = true): value{value}, updateState{updateState} {}
    );
    COMMAND(34, SetTime,
        TinyDateTime value;
        SetTime(TinyDateTime value): value{value} {}
    );

    /** Sets the alarm to given value and enables it. 
     */
    COMMAND(35, SetAlarm, 
        TinyAlarm value;
        SetAlarm(TinyAlarm value): value{value} {}
    );


    COMMAND(36, SetAudioSettings,
        AVRState::AudioSettings settings;
        SetAudioSettings(AVRState::AudioSettings const & settings): settings{settings} {}
    );

    /** Explicitly sets budget settings to given values. Note that setting the budget does not affect the reset counter. This command can be used to update the budget value when counting down, or top-up the budget when appropriate.
     */
    COMMAND(37, SetBudget,
        AVRState::BudgetSettings budget; 
        SetBudget(AVRState::BudgetSettings const & budget): budget{budget} {}
    );

    /** Decrements budget by single second. This message is sent to the AVR every second during which a budget counted app is active. This ensures that (a) if there is RP crash, the budget will be valid, and (b) at midnight, there will be no data race between budget update and budget decrement.
     */
    COMMAND(38, DecBudget);

    /** Resets the budget to its daily allowance. 
     */
    COMMAND(39, ResetBudget);

    
    COMMAND(40, RGBOff);

    /** Specifies the RGB effect for a particular LED. 
    */
    COMMAND(41, SetRGBEffect, 
        uint8_t index;
        RGBEffect effect;
        SetRGBEffect(uint8_t index, RGBEffect const & effect): index{index}, effect{effect} {}
    );

    /** Sets RGB effects for all LEDs. 
        
        As the DPAD contains multiple LEDs, the DPAD effect is applied to all DPAD LEDs at once. For finer control over the DPAD LEDs, eithe select the individual LEDs using SetRGBEffect, or use the SetRGBEffectDPAD command to control just the DPAD LEDs.
     */
    COMMAND(42, SetRGBEffects, 
        RGBEffect a;
        RGBEffect b;
        RGBEffect dpad;
        RGBEffect sel;
        RGBEffect start;
        SetRGBEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start):
            a{a}, b{b}, dpad{dpad}, sel{sel}, start{start} {}
    );

    /** Sets RGB effects on the DPAD LEDs alone.
     */
    COMMAND(43, SetRGBEffectDPAD, 
        RGBEffect topLeft;
        RGBEffect topRight;
        RGBEffect bottomLeft;
        RGBEffect bottomRight;
        SetRGBEffectDPAD(RGBEffect const & topLeft, RGBEffect const & topRight, RGBEffect const & bottomLeft, RGBEffect const & bottomRight):
            topLeft{topLeft}, topRight{topRight}, bottomLeft{bottomLeft}, bottomRight{bottomRight} {}
    );

    COMMAND(44, Rumbler,
        RumblerEffect effect;
        Rumbler(RumblerEffect const & effect): effect{effect} {}
    );

    COMMAND(45, SetPin, 
        uint16_t pin;
        SetPin(uint16_t pin): pin{pin} {}
    );

} // namespace rckid::cmd