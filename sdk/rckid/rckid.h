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

/** \page sdk
    \section memory Memory Management

    As the device has only a limited amount of RAM (~512 KB excluding stacks for mk III), some careful management is necessary. RCKid supports two dynamic allocation schemes, a normal heap and arena allocators.
    
    Stack Protection

    Another variable in the memory layout is relative small stack sizes (only 4kb per core). Those stacks can easily overflow and corrupt the heap, or one another. Stack protection scheme can be employed to detect stack overflow into a the heap and prevent further execution (recovery at this point is impossible). 

    To enable the protection, memoryInstrumentStackProtection() must be called first, which writes magic bytes to the end of the stack regions (just above RAM). Subsequent periodic calls to memoryCheckStaticProtection() then compare the memory to the expected magic value and will raise an error (error::StackProtectionFailure) upon failure. 

    NOTE that the stack protection scheme is only meaningful on the actual devices, the fantasy console uses the OS callstack instead and this will likely never overflow into the fantasy heap region. 

    \ref memoryFree, \ref Heap, \ref Arena, 
*/

#include "memory.h"
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

        The audio uses 16bit signed format and supports either mono, or stereo playback and mono recording via the buil-in microphone with sample rates of up to 48kHz. To conserve memory, both playback and recording require the app to supply the audio system with a double buffer that the audio system will use to cache and stream out the audio data, and a callback function. 

        The callback function for the playback takes a buffer and its size in *stereo samples* and should fill the buffer with up to the specified number of stereo samples, returning the number of stereo samples actually written (which can be smaller then the number of samples the buffer can hold). Internally, the device uses the double buffer so that when one part is being streamed out via DMA, the other part can be refilled by the application.

     */

    /** Audio callback to fill/empty audio buffers during playback/recording. During playback, stereo buffer is expected, and hence the second input argument is number of stereo samples and the rerurned value is the number of stereo samples written. 
     
        TODO recording
     */
    using AudioCallback = std::function<uint32_t(int16_t *, uint32_t)>;

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

    /** Starts playback of given buffer at spefified sample rate. It is expected the buffer is empty and the callbackfunction will be called automatically to populate it. 
     */
    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb);

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