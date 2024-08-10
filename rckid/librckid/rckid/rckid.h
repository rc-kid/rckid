#pragma once

#include <platform.h>

#include "rckid/common/state.h"
#include "errors.h"
#include "graphics/color.h"

#define TRACE(...) ::rckid::writeToSerial() << __VA_ARGS__ << "\r\n"
#define DEBUG(...) ::rckid::writeToSerial() << __VA_ARGS__ << "\r\n"

#define MEASURE_TIME(whereTo, ...) { \
    uint32_t start__ = uptimeUs(); \
    __VA_ARGS__; \
    whereTo = uptimeUs() - start__; \
}

namespace rckid {

    class AudioStream;

    /** \name Cartridge pins. 
     */
    //@{
    constexpr gpio::Pin GPIO14 = gpio::Pin{14};
    constexpr gpio::Pin GPIO15 = gpio::Pin{15};
    constexpr gpio::Pin GPIO16 = gpio::Pin{16};
    constexpr gpio::Pin GPIO17 = gpio::Pin{17};
    constexpr gpio::Pin GPIO18 = gpio::Pin{18};
    constexpr gpio::Pin GPIO19 = gpio::Pin{19};
    constexpr gpio::Pin GPIO20 = gpio::Pin{20};
    constexpr gpio::Pin GPIO21 = gpio::Pin{21};
    //@}


    /** Buttons. 
    */
    enum class Btn {
        Left, 
        Right,
        Up, 
        Down, 
        A, 
        B, 
        Select, 
        Start,
        Home, 
        VolumeUp, 
        VolumeDown,
    }; // rckid::Btn

    /** Initializes the SDK.
     
        This *must* be the first function a cartridge calls if it wants 
     */
    void initialize();

    /** Yields to the RCKid's device events. 
     
        TODO what events? can all be done in IRQ? Maybe the tinyUSB? 
     */
    void yield();

    /** Performs single tick. 

        During a tick, the framework talks to the AVR and attached sensors to determine the state of the attached peripherals. Since the I2C is very slow, performing the tick is actually expected to be done in parallel with the rendering and drawing and calling the tick function first finishes the previous tick, if any and then starts a new one, which will be handled v
     */
    void tick();

    /** \name Controls & Sensors
        
        RCKid controls comprise of a dpad, buttons A and B, buttons Select and Start, side home button and volume up & down keys. Furthermore, RCKid is equipped with a 3axis accelerometer and gyroscope as well as an ambient and UV light sensor. 

        Finally, the AVR and the accelerometer can also measure temperature. 

     */
    //@{

    /** Returns true if the given button is currently down. 
     */
    bool down(Btn b);

    /** Returns true if the given button has been pressed since last frame (i.e. its state went from up to down). The value is stable within one frame. */
    bool pressed(Btn b);

    /** Returns true if the given button has been released since last frame (i.e. its state went from down to up). The value is stable within one frame. */
    bool released(Btn b);

    /** Returns the accelerometer readings. 
     */
    int16_t accelX(); 
    int16_t accelY();
    int16_t accelZ();

    /** Returns the gyroscope readings. 
     */
    int16_t gyroX();
    int16_t gyroY();
    int16_t gyroZ();

    /** Returns the readout of the ambient light sensor. 
     */
    uint16_t lightAmbient();
 
    /** Returns the readout of the UV light sensor. 
     */
    uint16_t lightUV(); 

    //}

    /** \name Brightness, LEDs, notifications and rumbler. 
    */
    //@{

    /** Sets the LCD screen brightness. 
     
        0 for backlight off, 255 for maximum brightness. 
     */
    void setBrightness(uint8_t brightness);

    /** Disables all LEDs. 
     */
    void disableLEDs(); 

    /** Sets the LED effect for given button. Note that home button & volume keys do not have their own leds. 
     */
    void setButtonEffect(Btn btn, RGBEffect effect);

    /** Sets LED effects for all available buttons. 
     */
    void setButtonsEffects(RGBEffect a, RGBEffect b, RGBEffect dpad, RGBEffect sel, RGBEffect start);

    /** A rainbow RGB keyboard effect preset. 
     */
    inline void setButtonsRainbow(uint8_t brightness) {
        setButtonsEffects(
            RGBEffect::Rainbow(0, 5, 1, brightness),
            RGBEffect::Rainbow(10, 5, 1, brightness),
            RGBEffect::Rainbow(20, 5, 1, brightness),
            RGBEffect::Rainbow(30, 5, 1, brightness),
            RGBEffect::Rainbow(40, 5, 1, brightness)
        );
    }

    /** Sets the rumbler to given effect. 
     */
    void setRumbler(RumblerEffect effect);

    //@}

    /** \name Power Management
     */
    //@{

    void powerOff();

    /** Returns true if the device is currently charging the li-ion battery (must be in DC power mode). False when the charge has completed while still DC power attached, or running from batteries. 
     */
    bool charging();

    /** Returns true if the device is running from DC power, false when power from battery. 
     */
    bool dcPower();

    /** Returns the battery voltage in Volts x 100, i.e. full battery should read 420. 
     */
    unsigned vBatt(); 

    /** Returns the battery level in pct. 
     */
    unsigned batteryLevel();
    //@}

    //@}

    /** \name User Management 
     
        Each RCKid device belongs to a user, who can specify their name, accent color, image, birthday and so on. 
     */
    //@{

    //ColorRGB accentColor(); 

    //std::string const & username();

    //@}

    /** \name Memory Management
     
        Since RAM is quite constrained on the RP2040, RCKid implements its own malloc and free that are geared toards efficiency in memory allocation, not speed. Furthermore, the heap allows partitioning to arenas, where each arena can be used for both arena-like and heap-like allocations at the same time. When an arena is deallocated, all (including the heap-like allocated) its memory is freed. 
        
        This mechanism prevets framentation across applications as each application creates its own heap arena that will be completely deallocated when the application terminates. 
     */
    //@{
    size_t getFreeHeap();
    size_t getUsedHeap();
    size_t getMallocCalls();
    size_t getFreeCalls();
    void enterHeapArena();
    void leaveHeapArena();
    //@}


    /** \name Time Utilities
     */
    //@{

    /** Returns RP2040's uptime in microseconds. 
     */
    inline uint32_t uptimeUs() { return time_us_32(); }

    //@}

} // namespace rckid

extern "C" {
    void memFill32(uint32_t * buffer, uint32_t size, uint32_t value);
    void rckid_mem_fill_32x8(uint32_t * target, size_t num, uint32_t source); 
    uint8_t const * rckid_color256_to_rgb(uint8_t const * in, uint16_t * out, unsigned numPixels, uint16_t const * palette);
}

#include "../device_wrapper.h"