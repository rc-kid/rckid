#pragma once

#include "rckid.h"
#include "graphics/color.h"
#include "graphics/geometry.h"

namespace rckid {

    void onWakeup(uint32_t payload);
    void onPowerOff();
    void onSecondTick();
    void onFatalError(char const * file, uint32_t line, char const * msg, uint32_t payload);

} // namespace rckid

/** Hardware Abstraction Layer
 
    The functions defined in this file must be implemented for each and every platform supported by RCKid. They provide the most basic access to the hadrware features. 

    The AVR should be really simple - it keeps the basic data in memory and then offers stuff to read or write it via I2C commands.  Extra functions:

    log battery level changes, every 10m, this would give us 1 day worth of battery info in ~150bytes, which is likely ok
    Then it should allow access to RAM that can be used as scratch space for the RP2350. And maybe eeprom as well? What to store there? Device name & pin? 

 */

namespace rckid::hal {

    /** Represents the basic device state. 
     
        The state holds information about buttons and power state. This is a short value that is expected to be read very often (every tick) and contains all of the information needed to drive the basic functionality. 

        - 8 bits for dpad, a, b, sel, start buttons
        
        - 3 bits for home, vol up & down buttons
        - 1 bit wakeup interrupt
        - 1 bit accel interrupt
        - 1 bit poweroff interrupt
        - 1 bit second interrupt
        - 

        - 1 bit debug mode
        - 1 bit parent mode
        - 1 bit bootloader mode
        - 1 bit charging
        - 
        -
        -
        - 1 bit headphones connected

        - 7 bits for battery voltage level
        - 1 bit for usb power connected
     */
    class State {
    public:

        /** Returns the raw state value. 
         */
        uint32_t rawState() const {
            return (static_cast<uint32_t>(a_) << 0) |
                   (static_cast<uint32_t>(b_) << 8) |
                   (static_cast<uint32_t>(c_) << 16) |
                   (static_cast<uint32_t>(d_) << 24);
        }

        uint16_t buttons() const {
            return (static_cast<uint16_t>(a_) << 0) |
                   (static_cast<uint16_t>(b_) << 8);
        }

        /** Updates the device state with new state value. The operation is slightly more involved as the interrupt flags must be ORed in. 
         */
        void updateWith(State const & other) {
            // TODO
        }

        /** Sets state of given button explicitly. 
         
            Buttons are identified by their bitmask value as defined in the rckid::Btn enum.
         */
        void setButtonState(uint16_t btnId, bool state) {
            if (btnId > 255) {
                btnId >>= 8;
                state ? (b_ |= static_cast<uint8_t>(btnId)) : (b_ &= ~static_cast<uint8_t>(btnId));
            } else {
                state ? (a_ |= static_cast<uint8_t>(btnId)) : (a_ &= ~static_cast<uint8_t>(btnId));
            }
        }

    private:
        uint8_t a_ = 0; 
        uint8_t b_ = 0; 
        uint8_t c_ = 0;
        uint8_t d_ = 0;

    }; // rckid::hal::State

    static_assert(sizeof(State) == 4, "Required so that State can be read/written as a single 32bit value");

    class device {
    public:
        /** Initializes the hardware abstraction layer, and brings up the device.
         
            This must be the *first* HAL function called.
        */
        static void initialize();

        /** Powers the device off immediately
         
            This function does not return and powers the device off immediately. Any necessary cleanup should have been done before calling this function, e.g. by calling the rckid::powerOff() function, which performs the cleanup and then calls the hal::powerOff() function to actually cut the power.
        */
        static void powerOff();

        /** Puts the device to sleep. 
         
            When in sleep mode, the cartridge and all peripherals remain powered, but the CPU is put to sleep to save power. The only way to wake the device up is either via scheduled wakeup, or via home button press. When sleep is over, the function returns so that normal execution can continue.
        */
        static void sleep();

        /** Schedules wakeup with given timeout in seconds and optional payload.
         
            When the timeout expires, calls the onWakeup function with the given payload. If the device is turned off, then it will be first turned on and then the onWakeup function is called during the first tick() after wakeup. Only one wakeup can be active at a time, scheduling a new wakeup overrides any previously scheduled one. 

            Scheduling wakeup with 0 timeout should clear the wakeup.

            A wakeup can be parametrized with a 32bit payload. This allows to identify the wakeup source, etc.
        */
        static void scheduleWakeup(uint32_t timeoutSeconds, uint32_t payload = 0);

        /** This function is called by the SDK every app tick. 
         
            This allows the HAL to tap into the ticks and execute some periodic code every frame.
        */
        static void onTick();

        /** This function is called every time the SDK's yield() function is called. 
         
            This allows the HAL to perform any necessary background processing. Unlike onTick(), this function is not guaranteed to be called periodically. Since each tick also yields, onYield() is called at least as often as onTick(), but usually much more.
        */
        static void onYield();

        /** Fatal error. 
         
            Fatal error should stop all device functionality and only display the error. Note that the function can be called from all kinds of contexts, and care should be taken that it always displays the requested error information. This includes, but is not limited to situations with out of memory problems. 
         */
        static void fatalError(char const * file, uint32_t line, char const * msg, uint32_t payload);

        /** Returns a format writer than can be used to output debug information. 
         
            Useful for logging, mostly corresponds to either USB serial adapter, or direct serial out via cartridge pins on the device. 
         */
        static Writer debugWrite();

        /** Reads a byte from the debug input. 
         
            Useful for receiving commands from the debug console. Should be non-blocking and return 0 if no data is available. 
         */
        static uint8_t debugRead();
        
    }; // rckid::hal::device

    class time {
    public:

        /** Returns system uptime in microseconds. 
         */
        static uint64_t uptimeUs();

        /** Returns the current date and time. 
         */
        static TinyDateTime now();

    }; // rckid::hal::time

    class io {
    public:

        /** Returns the current device state.
         */
        static State state();
        
        static Point3D accelerometerState();

        static Point3D gyroscopeState();

    }; // rckid::hal::io

    /** Display access. 
     
        RCKid expects 320x240 pixel display with 16bpp depth (RGB565). To make matters more "complicated", most displays in this range are vertically oriented, i.e. 240x320. If display tearing is to be elliminated, it is not enough to simply rotate the display in the driver, but the display should be updated in its "native" order, i.e. column first starting with right-most column from top to bottom, then next column to the left, etc. This way the update shall always be before the display refresh beam. 
        
        The bulk of the SDK supports precisely this rendering, but the HAL must also provide an option to use the potentially tearing, but more common row-first approach. 

        In either modes, first the display must be enable and the update rectangle & direction specified. After this, subsequent calls to update() should push the pixels to the selected area. The updates are wrapped, i.e. after the whole selected rectangle is updated, the next pixel goes to the start of the rectangle again. 

        TODO should the API for display updates be changed to use the audio buffers as well? 
     */
    class display {
    public:
        static constexpr int16_t WIDTH = 320;
        static constexpr int16_t HEIGHT = 240;

        using Callback = std::function<void(Color::RGB565 * & buffer, uint32_t & bufferSize)>; 

        enum class RefreshDirection {
            ColumnFirst,
            RowFirst,
        }; 

        static void enable(Rect rect, RefreshDirection direction);

        static void disable();

        static void setBrightness(uint8_t value);

        static bool vSync();

        static void update(Callback callback);

        static bool updateActive();

    }; // rckid::hal::display

    /** Audio playback and recording.
     
        
        
     */
    class audio {
    public:

        /** Audio callback function. 
         */
        using Callback = std::function<void(int16_t * & buffer, uint32_t & stereoSamples)>;

        static void setVolumeHeadphones(uint8_t value);

        static void setVolumeSpeaker(uint8_t value);

        static void play(uint32_t sampleRate, Callback cb);

        static void recordMic(uint32_t sampleRate, Callback cb);

        static void recordLineIn(uint32_t sampleRate, Callback cb);

        static void pause();

        static void resume();

        static void stop();

    }; // rckid::hal::audio

    /** Filesystem access. 
     
        The SDK supports two "drives" - SD card, which stays in the device and can be shared between cartridges, and the cartridge filesystem, in which case part of the flash chip inside the cartridge is used as a LittleFS volume. 

        The APIs are tailored towards their expected hardware characteristics, i.e. a block API for the SD card and raw flash API for the cartridge with erase & write sizes and commands.

        Whenever the capacity function returns 0 (either blocks for SD or bytes for cartridge), the particular drive is presumed non-existent.
     */
    class fs {
    public:
        static uint32_t sdCapacityBlocks();

        static bool sdReadBlocks(uint32_t blockNum, uint8_t * buffer, uint32_t numBlocks);

        static bool sdWriteBlocks(uint32_t blockNum, uint8_t const * buffer, uint32_t numBlocks);

        // TODO async variants

        static uint32_t cartridgeCapacityBytes();

        static uint32_t cartridgeWriteSizeBytes();

        static uint32_t cartridgeEraseSize();

        static void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes);

        static void cartridgeWrite(uint32_t start, uint8_t const * buffer);

        static void cartridgeErase(uint32_t start);

    }; // rckid::hal::fs

    /** Heap configuration. 
     
        The HAL must provide heap boundaries and swap the global malloc & free functions to those using the allocators provided in the memory.h file, which is platform agnostic. The expectation here is that heap starts at the heapStart() address, which is *fixed* during the entire execution and extends up to heapEnd(), which may change depending on the platform (namely due to stack growth).
     */
    class memory {
    public:

        static uint8_t * heapStart();

        static uint8_t * heapEnd();

    }; // rckid::memory

} // namespace rckid