#pragma once

#include <platform.h>
#include <platform/writer.h>
#include <platform/buffer.h>
#include <platform/tinydate.h>

#include "tracing.h"
#include "memory.h"

#include "config.h"
#include "common.h"
#include "graphics/geometry.h"
#include "utils/fixedint.h"
#include "errors.h"

#define MEASURE_TIME(whereTo, ...) { \
    uint32_t start__ = uptimeUs(); \
    __VA_ARGS__; \
    whereTo = uptimeUs() - start__; \
}

/** \defgroup api API
 
 */

/** \defgroup backends Backends
 
    While \ref api provides an abstraction layer over the actual hardware, the library also contains a backend, which deals with the hardware details. This architecture allows the backends to be swapped, so that same RCKid cartridges can be run on multiple devices, or even emulators. 

    All backends are stored in `/sdk/backends` folder. 

    To create a new backend, implementation for all functions mentioned in the rckid.h file must be provided. These functions generally fall into the following categories:

    - user input detection (buttons, accelerometer, UV sensor)
    - feedback effects (LEDs, rumbler)
    - display
    - audio
    - memory (malloc & free)
    - filesystem (SD card)
  */

namespace rckid {

    class ColorRGB;

    /** Overclocks the device. 
     
        Must be called before initialize(). May not have an effect if overclocking is not supported by the current backend. 
     */
    void overclock(); 

    /** Initializes the RCKid console. 
     
        This must be the first SDK function called by the application. 
     */
    void initialize();

    void tick();

    void yield();

    /** Programatically resets the idle timer that automatically powers the device off. 
     
        Use with care only in cases where it makes sense (such as when playing media) otherwise this has obviously  very negative impact on the battery. 
     */
    void keepAlive();

    /** Returns the system's uptime in microseconds. 
     
        For performance reasons, this uses uint32_t as the result value and as such will overflow every hour & something. The intended purpose of this function is not precise timekeeping, but delta time measurements, so the overflows are fine. 
     */
    uint32_t uptimeUs();

    /** Returns current date and time as kept by the AVR & RPi. 
     */
    TinyDate dateTime();

    /** Sets the date & time kept by the device to the given value. 
     */
    void setDateTime(TinyDate value);

    /** Returns the currently set alarm.
     */
    TinyAlarm alarm();

    void setAlarm(TinyAlarm alarm);

    /** Generates random number in the 32bit unsigned range. 
     */
    uint32_t random();

    /** Returns debug writer for logging and tracing purposes. 
     
        Depending on the backend, this is either the serial over USB port for the physical devices, or standard output for the fantasy console. 
     */
    Writer debugWrite();

    /** \name Controls & Sensors
        
        RCKid controls comprise of a dpad, buttons A and B, buttons Select and Start, side home button and volume up & down keys. Furthermore, RCKid is equipped with a 3axis accelerometer and gyroscope as well as an ambient and UV light sensor. 

        The device also supports analogue joystick peripheral that can be physically controlled by either dpad, or the accelerometer. 
     */
    //@{

    /** Enum for the buttons available. 
     */
    enum class Btn : uint32_t {
        Left       = 1 << 0, 
        Right      = 1 << 1,
        Up         = 1 << 2, 
        Down       = 1 << 3, 
        A          = 1 << 4, 
        B          = 1 << 5, 
        Select     = 1 << 6, 
        Start      = 1 << 7,
        Home       = 1 << 8, 
        VolumeUp   = 1 << 9, 
        VolumeDown = 1 << 10,
    }; // rckid::Btn

    /** Returns true if the given button is currently down. 
     */
    bool btnDown(Btn b);

    /** Returns true if the given button has been pressed since last frame (i.e. its state went from up to down). The value is stable within one frame. */
    bool btnPressed(Btn b);

    /** Returns true if the given button has been released since last frame (i.e. its state went from down to up). The value is stable within one frame. */
    bool btnReleased(Btn b);

    /** Clears button press information
     
        Useful for supressing default button actions when the app uses own mechanics.
     */
    void btnPressedClear(Btn b);

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


    /** Fake joystick. 
     */
    int8_t joystickX();
    int8_t joystickY();
    inline int8_t joystickX(int8_t min, int8_t max) { 
        int8_t r = joystickX();
        return (r < min) ? min : ((r > max) ? max : r);
    }
    inline int8_t joystickY(int8_t min, int8_t max) { 
        int8_t r = joystickY();
        return (r < min) ? min : ((r > max) ? max : r);
    }

    void joystickXUseDPad(FixedInt acceleration = FixedInt{1});
    void joystickYUseDPad(FixedInt acceleration = FixedInt{1});
    inline void joystickUseDPad() {
        joystickXUseDPad();
        joystickYUseDPad();
    }
    inline void joustickUseDPad(FixedInt accelX, FixedInt accelY) {
        joystickXUseDPad(accelX);
        joystickYUseDPad(accelY);
    }


    /** Returns the readout of the ambient light sensor. 
     */
    uint16_t lightAmbient();
 
    /** Returns the readout of the UV light sensor. 
     */
    uint16_t lightUV(); 

    /** Temperature in tenths of degree Celsius as measured by the AVR firmware
     */
    int16_t tempAvr(); 

    //}

    /** \name Power Management
     */
    //@{

    /** Puts the device to sleep */
    void sleep();

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

    /** \name Display
     
       RCKid supports display with resolution of 320x240 pixels at 65536 colors. 
       
       The display on the device is 320x240 2.8" IPS display with 8080 8bit interface using ST7789 driver, but since the the display is originally vertical, 
     */
    //@{

    enum class DisplayMode {
        Native, 
        Natural,
        NativeDouble, 
        NaturalDouble,
        Off,
    }; 

    /** Callback function for display update. 
         
        The function takes no arguments. It may schedule another update, or return immediately if the update is finished for the current frame. 
     */
    using DisplayUpdateCallback = std::function<void()>;

    /** Returns the current display mode. 
     */
    DisplayMode displayMode(); 

    /** Sets the display mode. 
      
        Does nothing if the new mode is same as current mode. Setting display mode to DisplayMode::Off turns the display off, *including* the backlight, but brigtness setting itself will not be affected. 
     */
    void displaySetMode(DisplayMode mode);

    /** Returns the display brightness. 
     */
    uint8_t displayBrightness(); 

    /** Sets the display brightness.
     
        If the display is on, the effect should be immediate. 
     */
    void displaySetBrightness(uint8_t value);

    /** Returns the current update region of the display. 
     */
    Rect displayUpdateRegion();

    /** Sets the display update region. 
     */
    void displaySetUpdateRegion(Rect region);

    /** Sets the display update region based on width and height. The region will be automatically centered. 
     */
    inline void displaySetUpdateRegion(Coord width, Coord height) {
        switch (displayMode()) {
            case DisplayMode::Native:
            case DisplayMode::Natural:
                ASSERT(width >= 0 && width <= 320);
                ASSERT(height >= 0 && height <= 240);
                return displaySetUpdateRegion(Rect::XYWH((320 - width) / 2, (240 - height) / 2, width, height));
            case DisplayMode::NativeDouble:
            case DisplayMode::NaturalDouble:
                ASSERT(width >= 0 && width <= 160);
                ASSERT(height >= 0 && height <= 120);
                return displaySetUpdateRegion(Rect::XYWH((160 - width) / 2, (120 - height) / 2, width, height));
            case DisplayMode::Off:
                // although this is a bit weird...
                return;
        }
    }

    /** Returns true if display is currently being updated. 
     */
    bool displayUpdateActive(); 

    /** Busy loop convenience function that waits for the display update to be done. 
     */
    inline void displayWaitUpdateDone() {
        while (displayUpdateActive())
            yield();
    }

    /** Busy waits for the display's VSYNC 
     */
    void displayWaitVSync();

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback);

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels);

    inline void displayUpdateBlocking(ColorRGB const * pixels, uint32_t numPixels) {
        ASSERT(!displayUpdateActive() && "Blocking update must be first and the only one in a frame");
        displayUpdate(pixels, numPixels);
        // busy wait for the async update to finish
        while (displayUpdateActive())
            yield();
    }

    //@}

    /** \name Audio

        The audio uses 16bit signed format and supports either mono, or stereo playback and mono recording via the buil-in microphone with sample rates of up to 48kHz. To conserve memory, both playback and recording require the app to supply the audio system with a double buffer that the audio system will use to cache and stream out the audio data, and a callback function. 

        The callback function for the playback takes a buffer and its size in *stereo samples* and should fill the buffer with up to the specified number of stereo samples, returning the number of stereo samples actually written (which can be smaller then the number of samples the buffer can hold). Internally, the device uses the double buffer so that when one part is being streamed out via DMA, the other part can be refilled by the application.
     */
    //@{

    /** Enables the audio.
     */
    void audioOn();

    void audioOff();

    bool audioEnabled();

    bool audioHeadphones();

    int32_t audioVolume();

    void audioSetVolume(int32_t value);

    uint32_t audioSampleRate();

    /** Starts playback with given sample rate and callback function.
     */
    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, std::function<uint32_t(int16_t *, uint32_t)> cb);

    //void audioRecord(DoubleBuffer & data, uint32_t sampleRate = 8000);

    void audioPause();

    void audioStop();

    //@}

    /** \name RGB LEDs
     
        The top plate buttons (DPAD, A, B and Select and Start) each contain one RGB LED underneath that can be used for various light effects. There is also sixth LED in the upper display border that is used for notifications and is not expected to be changed by the application directly. 

        The RGBs support simple effects, such as breathe, rainbow hue, etc.  

        Since the LEDs are neopixels at 5 volts and together consume at least 6mA even if completelt black, it is important to turn them off via ledsOff() function whenever they are not needed. 
     */
    //@{

    /** Turns all the LEDs off to save power. 
     */
    void ledsOff(); 

    void ledSetEffect(Btn b, RGBEffect const & effect);

    void ledSetEffects(RGBEffect const & dpad, RGBEffect const & a, RGBEffect const & b, RGBEffect const & select, RGBEffect const & start); 

    //@}

    /** \name Rumbler 
     
        A simple rumbler interface that allows playing a rumbler effect that consists of N iterations of rumble with particular strength for a time followed by a pause.
     */
    //@{

    void rumble(RumblerEffect const & effect);

    inline void rumbleNudge() { rumble(RumblerEffect::Nudge()); }
    inline void rumbleOk() { rumble(RumblerEffect::Ok()); }
    inline void rumbleFail() { rumble(RumblerEffect::Fail()); }
    inline void rumbleAttention() {rumble(RumblerEffect::Attention()); }

    //@}

    /** \name SD Card Filesystem access. 
     
        RCKid device contains an SD card that can be used to store various data that can persist between different cartridges, such as music, messages, images, etc. The actual filesystem is handled by the FatFS library where the RCKid SDK only provides the necessary functions that enable the filesystem access. 

        NOTE: Those functions are *not* intended to be called from applications. Instead the filesystem namespace that provides the necessary filesystem abstractions onver the SD card basic read & write interface should be used. 
     */
    //@{

    /** Returns the capacity of the installed SD card in 512 byte blocks, or 0 if there is no SD card, or if its initialization has failed. 
     */
    uint32_t sdCapacity();

    /** Reads given number of blocks starting at provided offset from the SD card into the given buffer.
     */
    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks);

    /** Writes given number of blocks from the provided beginning offset. 
     */
    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks);

    //@}

    /** \name Cartridge Filsystem access. 
     
        Provides access to a section of the cartridge flash memory. Due to the nature of the flash memory on the device, three functions are provided, one for reading a block. The API is tailored for the NOR flash supported by RPI, hence minimal read size of 1 as the flash is memory mapped via XIP on the device, while the write page size and write block size are each different, generally at 256 and 4096 bytes. 

        On the fantasy backend, the flash is emulated by its own file. 
     */
    //@{
    uint32_t cartridgeCapacity();

    uint32_t cartridgeWriteSize();

    uint32_t cartridgeEraseSize();

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes);

    void cartridgeWrite(uint32_t start, uint8_t const * buffer);

    void cartridgeErase(uint32_t start);

    //@}

    /** \name Accelerated functions
     */
    //@{

    void memFill(uint8_t * buffer, uint32_t size, uint8_t value);
    void memFill(uint16_t * buffer, uint32_t size, uint16_t value);
    void memFill(uint32_t * buffer, uint32_t size, uint32_t value);

    //@}

} // namespace rckid
