#pragma once

#include <platform.h>
#include <platform/writer.h>
#include <platform/tinydate.h>

#include <rckid/graphics/color.h>
#include <rckid/graphics/geometry.h>

namespace rckid {

    enum class PowerMode {
        Normal,
        Boost,
    }; // rckid::PowerMode

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

    void onWakeup(uint32_t payload);
    void onPowerOff();
    [[noreturn]] void onFatalError(char const * file, uint32_t line, char const * msg, uint32_t payload);

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

        The first byte corresponds to the current state of the 8 control buttons (dpad, a, b, sel and start). 

        The second byte corresponds to side buttons (home, vol up and down) and device states (debug mode, parent mode, chargingm headphones, etc.)

        Third byte is current voltage. 

        Fourth byte is interrupts and is cleared specifically

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

        /** Updates the device state with new state value. The operation is slightly more involved as the interrupt flags must be ORed in. 
         */
        void updateWith(State const & other) {
            a_ = other.a_;
            b_ = other.b_;
            c_ |= other.c_;
            d_ = other.d_;
        }

        bool button(Btn btn) {
            uint16_t btnId = static_cast<uint16_t>(btn);
            if (btnId > 255)
                return (b_ & (btnId >> 8)) != 0;
            else
                return (a_ & btnId) != 0;
        }

        void setButton(Btn btn, bool state) {
            uint16_t btnId = static_cast<uint16_t>(btn);
            if (btnId > 255) {
                btnId >>= 8;
                state ? (b_ |= static_cast<uint8_t>(btnId)) : (b_ &= ~static_cast<uint8_t>(btnId));
            } else {
                state ? (a_ |= static_cast<uint8_t>(btnId)) : (a_ &= ~static_cast<uint8_t>(btnId));
            }
        }

        bool headphonesConnected() const { return (b_ & HEADPHONES_MASK) != 0; }
        bool charging() const { return (b_ & CHARGING_MASK) != 0; }


        void setHeadphonesConnected(bool connected) { connected ? (b_ |= HEADPHONES_MASK) : (b_ &= ~HEADPHONES_MASK); }
        void setCharging(bool charging) { charging ? (b_ |= CHARGING_MASK) : (b_ &= ~CHARGING_MASK); }


        /** Returns the current voltage in 100mV units (i.e. volts * 10). 

            The available range is 2.45V (250) to 5V (500). Anything higher than 5V will still read as 5V and anything below 2.45V will read as 0V. This is fine as the range is only used to determine li-ion batter levels (3.00V - 4.30V), or USB power plugged in (5.00V).
         */
        uint32_t vcc() const { return (d_ == 0) ? 0 : (d_ + 245); }

        void setVcc(uint16_t vx100) {
            if (vx100 < 245)
                d_ = 0;
            else if (vx100 > 500)
                d_ = 255;
            else
                d_ = static_cast<uint8_t>(vx100 - 245);
        }

    private:
        /** MSB ....... LSB
            | | | | | | | |
            | | | | | | | --- dpad left
            | | | | | | ----- dpad right
            | | | | | ------- dpad up
            | | | | --------- dpad down
            | | | ----------- a
            | | ------------- b
            | --------------- select
            ----------------- start 
         */
        uint8_t a_ = 0; 
        /** MSB ....... LSB
            | | | | | | | |
            | | | | | | | --- home
            | | | | | | ----- volume up
            | | | | | ------- volume down
            | | | | --------- headphones connected
            | | | ----------- charging
            | | ------------- 
            | --------------- 
            -----------------  
         */
        uint8_t b_ = 0; 
        static constexpr uint8_t HEADPHONES_MASK = 1 << 3;
        static constexpr uint8_t CHARGING_MASK = 1 << 4;
        /** MSB ....... LSB (interrupts)
            | | | | | | | |
            | | | | | | | --- 
            | | | | | | ----- 
            | | | | | ------- 
            | | | | --------- 
            | | | ----------- accel interrupt
            | | ------------- wakeup interrupt
            | --------------- power off interrupt
            -----------------  
         */
        uint8_t c_ = 0;
        /** Voltage 
         */
        uint8_t d_ = 0;

    }; // rckid::hal::State

    static_assert(sizeof(State) == 4, "Required so that State can be read/written as a single 32bit value");

    namespace device {
        /** Initializes the hardware abstraction layer, and brings up the device.
         
            This must be the *first* HAL function called.
        */
        void initialize();
        
        /** Sets the device power mode. 
         
            Under the hood, the boost power mode corresponds the CPU to highest sustainable frequency. Has no effect on the fantasy console.
         */
        void setPowerMode(PowerMode mode);

        /** Powers the device off immediately
         
            This function does not return and powers the device off immediately. Any necessary cleanup should have been done before calling this function, e.g. by calling the rckid::powerOff() function, which performs the cleanup and then calls the hal::powerOff() function to actually cut the power.
        */
        [[noreturn]] void powerOff();

        /** Puts the device to sleep. 
         
            When in sleep mode, the cartridge and all peripherals remain powered, but the CPU is put to sleep to save power. The only way to wake the device up is either via scheduled wakeup, or via home button press. When sleep is over, the function returns so that normal execution can continue.
        */
        void sleep();

        /** Schedules wakeup with given timeout in seconds and optional payload.
         
            When the timeout expires, calls the onWakeup function with the given payload. If the device is turned off, then it will be first turned on and then the onWakeup function is called during the first tick() after wakeup. Only one wakeup can be active at a time, scheduling a new wakeup overrides any previously scheduled one. 

            Scheduling wakeup with 0 timeout should clear the wakeup.

            A wakeup can be parametrized with a 32bit payload. This allows to identify the wakeup source, etc.
        */
        void scheduleWakeup(uint32_t timeoutSeconds, uint32_t payload = 0);

        /** This function is called by the SDK every app tick. 
         
            This allows the HAL to tap into the ticks and execute some periodic code every frame.
        */
        void onTick();

        /** This function is called every time the SDK's yield() function is called. 
         
            This allows the HAL to perform any necessary background processing. Unlike onTick(), this function is not guaranteed to be called periodically. Since each tick also yields, onYield() is called at least as often as onTick(), but usually much more.
        */
        void onYield();

        /** Fatal error. 
         
            Fatal error should stop all device functionality and only display the error. The purpose of the function is to restore the device state into something that can be operated on safely and then call the onFatalError() SDK handler than performs the actual error reporting. 
         */
        [[noreturn]] void fatalError(char const * file, uint32_t line, char const * msg, uint32_t payload);
        // can't use default arguments because of multiple declarations
        [[noreturn]] inline void fatalError(char const * file, uint32_t line, char const * msg_) {
            fatalError(file, line, msg_, 0);
        }
        
        /** Returns a format writer than can be used to output debug information. 
         
            Useful for logging, mostly corresponds to either USB serial adapter, or direct serial out via cartridge pins on the device. 
         */
        Writer debugWrite();

        /** Reads a byte from the debug input. 
         
            Useful for receiving commands from the debug console. Should be non-blocking and return 0 if no data is available. 
         */
        uint8_t debugRead();
        
    } // namespace rckid::hal::device

    namespace time {

        /** Returns system uptime in microseconds. 
         */
        uint64_t uptimeUs();

        /** Returns the current date and time. 
         */
        TinyDateTime now();

        /** Sets current time & date. 
         */
        void setTime(TinyDateTime dt);

    } // namespace rckid::hal::time

    namespace io {

        /** Returns the current device state.
         */
        State state();
        
        Point3D accelerometerState();

        Point3D gyroscopeState();

    } // namespace rckid::hal::io

    /** Display access. 
     
        RCKid expects 320x240 pixel display with 16bpp depth (RGB565). To make matters more "complicated", most displays in this range are vertically oriented, i.e. 240x320. If display tearing is to be elliminated, it is not enough to simply rotate the display in the driver, but the display should be updated in its "native" order, i.e. column first starting with right-most column from top to bottom, then next column to the left, etc. This way the update shall always be before the display refresh beam. 
        
        The bulk of the SDK supports precisely this rendering, but the HAL must also provide an option to use the potentially tearing, but more common row-first approach. 

        In either modes, first the display must be enable and the update rectangle & direction specified. After this, subsequent calls to update() should push the pixels to the selected area. The updates are wrapped, i.e. after the whole selected rectangle is updated, the next pixel goes to the start of the rectangle again. 
     */
    namespace display {

        constexpr Coord WIDTH = 320;
        constexpr Coord HEIGHT = 240;

        /** Buffer rendering callback.
         */
        using Callback = std::function<void(Color::RGB565 * & buffer, uint32_t & bufferSize)>; 

        enum class RefreshDirection {
            ColumnFirst,
            RowFirst,
        }; 

        void enable(Rect rect, RefreshDirection direction); 

        void disable();

        void setBrightness(uint8_t value);

        bool vSync();

        void update(Callback callback);

        void update(Color::RGB565 const * buffer, uint32_t bufferSize);

        void update(Color::RGB565 const * buffer1, uint32_t bufferSize1, Color::RGB565 const * buffer2, uint32_t bufferSize2);

        bool updateActive();

    } // namespace rckid::hal::display

    /** Audio playback and recording.
     
        Audio volume is 0..15
        
        
     */
    namespace audio {

        /** Audio callback function. 
         */
        using Callback = std::function<void(int16_t * & buffer, uint32_t & stereoSamples)>;

        void setVolumeHeadphones(uint8_t value);

        void setVolumeSpeaker(uint8_t value);

        void play(uint32_t sampleRate, Callback cb);

        void recordMic(uint32_t sampleRate, Callback cb);

        void recordLineIn(uint32_t sampleRate, Callback cb);

        void pause();

        void resume();

        void stop();

        bool isPlaying();

        bool isRecording();

        bool isPaused();

    } // namespace rckid::hal::audio

    /** Filesystem access. 
     
        The SDK supports two "drives" - SD card, which stays in the device and can be shared between cartridges, and the cartridge filesystem, in which case part of the flash chip inside the cartridge is used as a LittleFS volume. 

        The APIs are tailored towards their expected hardware characteristics, i.e. a block API for the SD card and raw flash API for the cartridge with erase & write sizes and commands.

        Whenever the capacity function returns 0 (either blocks for SD or bytes for cartridge), the particular drive is presumed non-existent.
     */
    namespace fs {

        uint32_t sdCapacityBlocks();

        void sdReadBlocks(uint32_t blockNum, uint8_t * buffer, uint32_t numBlocks);

        void sdWriteBlocks(uint32_t blockNum, uint8_t const * buffer, uint32_t numBlocks);

        // TODO async variants

        uint32_t cartridgeCapacityBytes();

        uint32_t cartridgeWriteSizeBytes();

        uint32_t cartridgeEraseSizeBytes();

        void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes);

        void cartridgeWrite(uint32_t start, uint8_t const * buffer, uint32_t numBytes);

        void cartridgeErase(uint32_t start);

    } // namespace rckid::hal::fs

    /** Heap configuration. 
     
        The HAL must provide heap boundaries and swap the global malloc & free functions to those using the allocators provided in the memory.h file, which is platform agnostic. The expectation here is that heap starts at the heapStart() address, which is *fixed* during the entire execution and extends up to heapEnd(), which may change depending on the platform (namely due to stack growth).
     */
    namespace memory {

        uint8_t * heapStart();

        uint8_t * heapEnd();

        bool isImmutableDataPtr(void const * ptr);

    } // namespace rckid::hal::memory

    /** Persistent storage access.
    
        1kb available for storage of device specific data. On the device, this is stored in the AVR's memory, but the AVR does not really understand the data. It just provides a simple read/write access to it. 
     */
    namespace storage {
        void load(uint16_t start, uint8_t * buffer, uint32_t numBytes);
        void save(uint16_t start, uint8_t const * buffer, uint32_t numBytes);
    } // namespace rckid::hal::store


} // namespace rckid