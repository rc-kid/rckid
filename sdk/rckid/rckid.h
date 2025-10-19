#pragma once

#include <platform.h>
#include <platform/writer.h>
#include <platform/tinydate.h>

#include <backend_config.h>

/** \page sdk RCKid SDK
 
    RCKid provides custom API to abstract from the hardware details of the console and to provide common functionality useful for 

    \section Backends

    Most of the API provided abstract from the hardware details of the device (this is not true for specific cartridge interfaces) so that as long as the SDK is used for HW access, different physical hardware can be used, including fantasy consoles. To this end the hardware specific functionality of the SDK is implemented in different _backends_.

    \subpage backend_mk3
    \subpage backend_mk2
    \subpage backend_fantasy
 */

 /** \page sdk
     \section peripherals Peripherals & Cartridge Drivers

     The SDK provides various backend & cartridge specific peripherals and drivers that can be used to access the specific hardware, such as FM radio, WiFi, NRF & LoRa cartridges, etc. Those are not part of this header file, but are specific classes with singleton instances that must be checked for their presentce before use (see the radio.h for an example). 
  */

/** \page sdk 
    \section err_log Errors and Logging

    TODO error handling

    Logging in RCKid is quite simple, but versatile. At its heart is the debugWrite() function which returns a writer used for the logging. This writer prints to the USB to UART converter, or standard output for the fantasy console. All logging should be done via the LOG macro, which takes two arguments - log level to be used, which can be any identifier and an expression to be logged. 

    Loglevels can be enabled or disabled at compile time so that there is no runtime cost associated with disabled loglevels both in performance and flash usage. To enable a log level, RCKid must be compiled with macro of the same name as the log level being set to `1`. 

    The following log levels are used by the SDK itself:

    - `LL_ERROR` is used for error that are not fatal. Is enabled unless ecplicitly silenced with `-DLL_ERROR=0`
    - `LL_WARN` is used for warnings, i.e. something that likely should not happen, but which does not interfere with device's functionality on its own. Is on by default
    - `LL_INFO` is for general information. Is enabled by default in non-release builds
 */
#include "error.h"
#include "log.h"

#include "utils/string.h"
#include "utils/buffers.h"

/** Measures the time it takes to compute the statements in the arguments in microseconds (returned as uint32_t). 
 */
#define MEASURE_TIME(...) [&]() { \
    uint32_t start__ = uptimeUs(); \
    __VA_ARGS__; \
    return uptimeUs() - start__; \
}()

/** Include headers required for the rckid API.
 */
#include "graphics/color.h"
#include "graphics/geometry.h"
#include "effects.h"

namespace rckid {

    /** Initializes the RCKid console. 
     
        This must be the first SDK function called by the application. Its task is to set up the console and completely depends on the backend used - while the device backends initialize the actual hardware after power on, the fantasy backend sets up the console window, etc. 
     */
    void initialize(int argc, char const * argv[]);
    inline void initialize() { initialize(0, nullptr); }

    void tick();

    void yield();

    /** Puts the device to sleep. */
    void sleep();

    /** Powers the device off.
     */
    void powerOff();

    /** Returns the current system voltage in 10mV increments, i.e. value of 500 corresponds to 5 volts, 2370 to 3.7 volts, etc.
     */
    uint16_t powerVcc();

    /** Returns true if the USB cable is connected, i.e. if the device is powered via DC and not battery. 
     */
    bool powerUsbConnected();

    /** Returns true if the device is currently charging the battery.
     
        This can only return true if the usb is connected. 
     */
    bool powerCharging();

    /** Programatically resets the idle timer that automatically powers the device off. 
    
        Internally, there are two counters - idle and keepalive countdowns. Both counters are reset at any button press, and the idle counter can also be reset by calling the keepAlive() function, effectively switching to the longer keepalive timeout for device power off. This is particularly useful for media playback, when no user feedback is necessary for extended periods of time.

        Use with care only in cases where it makes sense (such as when playing media) otherwise this has obviously  very negative impact on the battery. 
     */
    void keepAlive();

    /** Returns the system's uptime in microseconds. 
     
        For performance reasons, this uses uint32_t as the result value and as such will overflow every hour & something. The intended purpose of this function is not precise timekeeping, but delta time measurements, so the overflows are fine. 
     */
    uint32_t uptimeUs();

    /** Returns system uptime in microseconds with greater resolution. 
     */
    uint64_t uptimeUs64();

    /** Returns the current time as measured by the device. 
     */
    TinyDateTime timeNow();

    /** Returns the current alarm settings. 
     */
    TinyAlarm timeAlarm();

    /** Sets alarm. 
     */
    void setTimeAlarm(TinyAlarm alarm);

    /** \page sdk
        \section io IO

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

    /** Clears the pressed state of given button. 
     
        Useful for marking button event as processed so that further button press checks in the same period will not fire. Only works for the pressed and released events, button down state is not affected as it always reflects the current state of the button.
     */
    void btnClear(Btn b);

    /** Clears pressed button states for *all* buttons. 
     */
    void btnClear();

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

    uint16_t lightAmbient();
    uint16_t lightUV();

    /** \page sdk
        \section display Display Driver


     */

    /** Callback function for display update. 
         
        The function takes no arguments. It may schedule another update, or return immediately if the update is finished for the current frame. 
     */
    using DisplayUpdateCallback = std::function<void()>;

    enum class DisplayRefreshDirection {
        ColumnFirst,
        RowFirst,
    }; 

    void displayOn();

    void displayOff();

    void displayClear(ColorRGB color = ColorRGB::Black());

    DisplayRefreshDirection displayRefreshDirection();

    void displaySetRefreshDirection(DisplayRefreshDirection value);

    /** Returns the display brightness. 
     */
    uint8_t displayBrightness();

    /** Sets the display brightness 
     */
    void displaySetBrightness(uint8_t value); 

    /** Returns the display update region. The rectangle values must be interpreted in the current display resolution.
    */
    Rect displayUpdateRegion(); 

    /** Sets the display update region. The coordinates must be interpreter in the current display resolution.
     */
    void displaySetUpdateRegion(Rect value);

    /** Sets the display update region to a rectangle fo given width and height that will be centered on the screen. The width and height are interpreted in the current display resolution.
     */
    void displaySetUpdateRegion(Coord width, Coord height); 

    /** Returns true if the display is currently being updated. False otherwise.
     */
    bool displayUpdateActive();

    /** Waits until the display update is done.
     */
    void displayWaitUpdateDone();

    void displayWaitVSync();

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels, DisplayUpdateCallback callback);

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels);

    /** \page sdk
        \section audio Audio Playback & Recording

        The audio uses 16bit signed format and supports stereo playback and recording. The audio system is very simple and both playback and recording are done via DMA and require at least double buffering with the ability to use tripple or even larger buffers if necessary.

        This is done by the configurable callback function that is takes a reference to a sample buffer and its size (always in the number of stereo samples). The input arguments are always a buffer and its size that is being released by the audio system and can be re-used (for playback), or consumed (for recording) by the application. As both the buffer and its by reference the callback should also provide a new buffer and its size that will be used next by the audio system. At the beginning of playback, the callback is called twice to get the initial buffers required for double buffer - in case of recording those buffers will then be filled with the audio and retired via the callback function. 

        Internally, the audio uses pio to drive the codec with I2C with a double buffer confirguration and and back-to-back chained two DMA channels, i.e. while the callback for buffer 0 retirement/processing is being called, the second buffer is being processed by the codec.

        This provides seamless buffer transition, but requires that the callback function for playback provides the next buffer and especially its size immediately as the DMA reconfiguration happens immediately after the callback. So if the buffer sizes are always the same (such as the MP3 format), double buffering is enough (size stays the same, and the data can be filled later via the main loop as long as it happens before the other buffer is retired). Buf if frame sizes differ, then tripple buffering is necessary so that while one buffer is being played, another is being retired and the third one is already ready to be swapped for (via the reference parameters). 
     */

    /* Audio callback function that when called gives a buffer buffer that has been used, and expects the buffer to change to new buffer, returning the number of samples in the new buffer. */ 
    using AudioCallback = std::function<void(int16_t * &, uint32_t &)>;

    /** Returns true if headphones are connected. 
     */
    bool audioHeadphones(); 

    /** Reuturns true if audio processing is paused (playback or recording). 
     */
    bool audioPaused();

    /** Returns true if playback is active or paused.
     */
    bool audioPlayback();

    /** Returns true if recording is active or paused.
     */
    bool audioRecording();

    /** Returns the current audio volume. 
     */
    uint8_t audioVolume();

    /** Sets the current audio volume. 
     */
    void audioSetVolume(uint8_t value);

    /** Starts playback with the given sample rate and callback function.
     */
    void audioPlay(uint32_t sampleRate, AudioCallback cb);

    /** Starts recording the microphone at specified sample rate. When the buffer is filled, the callback function will be called so that the data can be processed. 
     */
    void audioRecordMic(uint32_t sampleRate, AudioCallback cb);

    /** Starts recording the line in input (FM radio on the device)  at specified rate. The callback function is called everytime the buffer is filled.
     */
    void audioRecordLineIn(uint32_t sampleRate, AudioCallback cb);

    /** Pauses audio playback or recording. 
     */
    void audioPause();

    /** Resumes audio playbnack or recoding. 
     */
    void audioResume();

    /** Stops playback or recording. 
     */
    void audioStop();

    /** \page sdk
        \section memory Memory Management

        As the device has only a limited amount of RAM (~512 KB excluding stacks for mk III), some careful management is necessary. RCKid supports two dynamic allocation schemes, a normal heap and arena allocators.
        
        Stack Protection

        Another variable in the memory layout is relative small stack sizes (only 4kb per core). Those stacks can easily overflow and corrupt the heap, or one another. Stack protection scheme can be employed to detect stack overflow into a the heap and prevent further execution (recovery at this point is impossible). 

        To enable the protection, memoryInstrumentStackProtection() must be called first, which writes magic bytes to the end of the stack regions (just above RAM). Subsequent periodic calls to memoryCheckStaticProtection() then compare the memory to the expected magic value and will raise an error (error::StackProtectionFailure) upon failure. 

        NOTE that the stack protection scheme is only meaningful on the actual devices, the fantasy console uses the OS callstack instead and this will likely never overflow into the fantasy heap region. 

        \ref memoryFree, \ref Heap, \ref Arena, 
    */

    /** Returns the unclaimed memory, i.e. the memory that can be taken up by either stack, or heap.
     */
    uint32_t memoryUnclaimed(); 

    /** Returns the number of free memory. 
     
        This is the unclaimed memory plus sum of all free chunks claimed by the heap (i.e. those in the middle of the heap). As this function walks the freelist, it may be "expensive". 
     */
    uint32_t memoryFree();
    
    /** Returns true if the memory comes from immutable region (ROM on the device)
     */
    bool memoryIsImmutable(void const * ptr);


    /** \page sdk
        \section sdfs SD Card Filesystem access. 
     
        RCKid device contains an SD card that can be used to store various data that can persist between different cartridges, such as music, messages, images, etc. The actual filesystem is handled by the FatFS library where the RCKid SDK only provides the necessary functions that enable the filesystem access. 

        NOTE: Those functions are *not* intended to be called from applications. Instead the filesystem namespace that provides the necessary filesystem abstractions onver the SD card basic read & write interface should be used. 
     */

    /** Returns the capacity of the installed SD card in 512 byte blocks, or 0 if there is no SD card, or if its initialization has failed. 
     */
    uint32_t sdCapacity();

    /** Reads given number of blocks starting at provided offset from the SD card into the given buffer.
     */
    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks);

    /** Writes given number of blocks from the provided beginning offset. 
     */
    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks);

    /** \page sdk
        \section cartfs Cartridge Filsystem access. 
     
        Provides access to a section of the cartridge flash memory. Due to the nature of the flash memory on the device, three functions are provided, one for reading a block. The API is tailored for the NOR flash supported by RPI, hence minimal read size of 1 as the flash is memory mapped via XIP on the device, while the write page size and write block size are each different, generally at 256 and 4096 bytes. 

        On the fantasy backend, the flash is emulated by its own file. 
     */
    uint32_t cartridgeCapacity();

    uint32_t cartridgeWriteSize();

    uint32_t cartridgeEraseSize();

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes);

    void cartridgeWrite(uint32_t start, uint8_t const * buffer);

    void cartridgeErase(uint32_t start);


    /** \page sdk
        \section rumbler Rumbler

        RCKid supports a simple rumbler inyterface, where the rumbler motor can be turned on (with given intensity), off, or a specific effect can be played. The effect is a simple repeated on/off cycle with given strength, time on, time off and number of cycles. The rumbler can be used to signal various events, such as notifications, or to provide haptic feedback in games.
     */
    //@{
    void rumblerEffect(RumblerEffect const & effect);
    
    inline void rumblerOn(uint8_t strength) {
        rumblerEffect(RumblerEffect{strength, 0, 0, 0});
    }

    inline void rumblerOff() {
        rumblerEffect(RumblerEffect::Off());
    }
    //@}

    /** \page sdk
        \section rgb RGB LEDs

        RCKid has an RGB LED under every front facing buttons. Together those LEDs can be used to signal various events, or to provide visual feedback in games. The LEDs can be set to a specific color, or a specific effect can be played. The effects support animations, such as breathing, rainbow, etc. The effects are defined in the rckid::RGBEffect class. 
     */
    //@{

    void rgbEffect(uint8_t rgb, RGBEffect const & effect);

    void rgbEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start);
    
    void rgbOff();

    //@}

    /** \page sdk
        \section user User Information
     

     */
    //@{

    /** Returns currently available budget in seconds. 
     */
    uint32_t budget();

    /** Returns the daily budget allowance set on the device in seconds. This is the value to which the budget is reset every day at midnight.
     */
    uint32_t budgetDaily();

    /** Sets the budget to given value. 
     */
    void budgetSet(uint32_t value);

    /** Sets the daily budget allowance to the given value. 
     */
    void budgetDailySet(uint32_t value);

    /** Performs manual budget allowance reset, i.e. what the device does automatically at midnight.
     */
    void budgetReset();
    //@}

} // namespace rckid

extern "C" {
    void memset8(uint8_t * buffer, uint8_t value, uint32_t size);
    void memset16(uint16_t * buffer, uint16_t value, uint32_t size);
    void memset32(uint32_t * buffer, uint32_t value, uint32_t size);
}