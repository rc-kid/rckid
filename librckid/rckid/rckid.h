#pragma once

#include <platform.h>
#include "common/config.h"
#include "common/state.h"
#include "common/commands.h"
#include "definitions.h"

#define UNREACHABLE
#define ASSERT(...)



#define CALCULATE_TIME(...) [&](){ uint32_t start__ = time_us_32(); __VA_ARGS__; return static_cast<unsigned>(time_us_32() - start__); }()


namespace rckid {

    /** Initializes the SDK.
     
        This *must* be the first function a cartridge calls if it wants 
     */
    void initialize();

    /** Yields to the RCKid's device events. 
     */
    void yield();

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

    /** Returns temperature in Celsius x 10 as measured by the AVR chip. 
     */
    unsigned tempAvr();

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

    /** Returns the VCC voltage in Volts x 100. 

        The VCC voltage should either be almost identical to the battery voltage if running on batterier, or be close to 5.0 V (value 500) when running on DC power. Note that this voltage is not available on the cartridge, which only supports the converted 3V3 user by the display and the RP2040 chip. 
     */
    unsigned vcc();

    /** Returns the battery voltage in Volts x 100, i.e. full battery should read 420. 
     */
    unsigned vBatt(); 
    //@}

    /** \name Memory Management
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


}

extern "C" {
    void rckid_mem_fill_32x8(uint32_t * target, size_t num, uint32_t source); 
}

#include "../device_wrapper.h"