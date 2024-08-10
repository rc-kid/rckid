#ifndef COMMAND
#error "COMMAND macro must be defined before including"
#endif

COMMAND(=0x00, Nop)
COMMAND(, PowerOff)
COMMAND(, Sleep)
COMMAND(, ResetRP)
COMMAND(, ResetAVR)
COMMAND(, BootloaderRP)
COMMAND(, BootloaderAVR)
COMMAND(, DebugModeOn)
COMMAND(, DebugModeOff)
COMMAND(, AudioOn)
COMMAND(, AudioOff)
COMMAND(, SetBrightness, 
    uint8_t value;
    SetBrightness(uint8_t value): value{value} {}
)
COMMAND(, SetTime,
    TinyDate value;
    SetTime(TinyDate value): value{value} {}
)

/** Copies the number of AVR errors (16 bit unsigned) to the comms buffer.
 */
COMMAND(, GetNumAVRErrors)

/** Copies the low byte of number of AVR errors and 16 last errors to the comms buffer. 
 */
COMMAND(, GetAVRErrors)

/** Clears the deviceError flag.
 */
COMMAND(, ClearAVRDeviceError)

/** Clears all last error information and resets the AVR error counter to 0. 
 */
COMMAND(, ResetAVRErrors)


COMMAND(, Rumbler,
    RumblerEffect effect;
    Rumbler(RumblerEffect effect): effect{effect} {}
)

/** Turns all LEDs off immediately. 
 */
COMMAND(, RGBOff)

/** Specifies the RGB effect for a particular LED. 
*/
COMMAND(, SetRGBEffect, 
    uint8_t index;
    RGBEffect effect;
    SetRGBEffect(uint8_t index, RGBEffect const & effect): index{index}, effect{effect} {}
)

COMMAND(, SetRGBEffects, 
    RGBEffect a;
    RGBEffect b;
    RGBEffect dpad;
    RGBEffect sel;
    RGBEffect start;
    SetRGBEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start):
        a{a}, b{b}, dpad{dpad}, sel{sel}, start{start} {}
)

#undef COMMAND