#pragma once

#include <platform.h>
#include <platform/tinydate.h>

#include <rckid/error.h>
#include <rckid/log.h>
#include <rckid/hal.h>

#define RCKID_DEFAULT_ANIMATION_DURATION_MS 500

#define RCKID_DEFAULT_KEY_REPEAT_MS 500

/** RCKid SDK

 */


namespace rckid {

    // device

    /** Initializes the RCKid SDK. 
     
        This function must be the first SDK function called as it initializes the underlying hardware and SDK abstractions. 
     */
    void initialize();

    /** Should be called periodically every app frame. 
     
        This corresponds to roughly 60 fps in the default setting. 
     */
    void tick(); 

    /** Yields execution to other tasks. 
        
        This function allows other tasks to run and should be called periodically from long-running operations to keep the system responsive. It is guaranteed to run at least once every tick.
     */
    inline void yield() { hal::device::onYield(); }

    // io

    /** Returns the current state of given button.
     
        True means the button has been pressed.
     */
    bool btnDown(Btn btn);

    /** Returns true if the button has been pressed. 
     
        Button press is registered if the button transitioned from up to down state between last & current tick.
     */
    bool btnPressed(Btn btn);

    /** Returns true if the button has been released.

        Button release is registered if the button transitioned from down to up state between last & current tick.
     */
    bool btnReleased(Btn btn);

    /** Clears the pressed/released state of the given button.
     
        After this call, btnPressed() and btnReleased() will not report the button as pressed or released until the next state change. Useful for silencing the change for further checks down the line. Does not change the current button value.
     */
    void btnClear(Btn btn);

    uint32_t btnRepeat(Btn btn);

    void btnSetRepeat(Btn btn, uint32_t repeat_ms);

    /** Clears all button state events. 
     */
    void btnClearAll();

    // TODO rapid fire


    /** Power management functions & monitoring. 
     */
    namespace power {

        /** Returns the current system voltage. This is the voltage from which the DC regulators are powered and corresponds to either the battery when running on battery, or the USB cable when power is plugged in. 
        */
        uint32_t vcc();

        /** Returns the current battery level percentage.
         */
        uint32_t batteryLevel();

        /** Returns true if the device is charging.
         */
        bool charging();

        /** Returns true if the device is connected to external DC power (via the USB connector), whether actively charging or not.
         */
        bool dcConnected();

    } // rckid::power


    // time

    namespace time {

        /** Returns system uptime in microseconds. 
         
            The function is a direct wrapper over hal::time::uptimeUs().
        */
        inline uint64_t uptimeUs() { return hal::time::uptimeUs(); }

        /** Returns current date & time.
         */
        TinyDateTime now();

    } // namespace time


    // debugging

    namespace debug {

        /** Returns writer for debugging purposes. 
         
            This is a direct wrapper over the hal::device::debugWrite() function that allows writing dbeug information to preferred debug outputs, such as USB serial adpater, or direct serial output via cartridge pins.
        */
        inline Writer write() { return hal::device::debugWrite(); }

        /** Reads characters from debug input.
         
            The call is non-blocking and returns 0 if no data is available. The function is a direct wrapper over the hal::device::debugRead() function.

            TODO should this return reader instead? 
        */
        inline uint8_t read() { return hal::device::debugRead(); }

        uint8_t readHex8();

        uint16_t readHex16();

    }

    /** Display manipulation.
        
     */
    namespace display {

        using RefreshDirection = hal::display::RefreshDirection;
        using Callback = hal::display::Callback;

        static constexpr Coord WIDTH = hal::display::WIDTH;
        static constexpr Coord HEIGHT = hal::display::HEIGHT;
            
        inline void waitUpdateDone() {
            while (hal::display::updateActive())
                yield();
        }

        inline void waitVSync() {
            while (hal::display::vSync())
                yield();
            while (! hal::display::vSync())
                yield();
        }

        void enable(Rect rect, RefreshDirection  direction);

        uint8_t brightness();

        void setBrightness(uint8_t value);
    
    } // namespace rckid::display

    namespace audio {

        using Callback = hal::audio::Callback;
        class DecoderStream;

        bool headphonesConnected();

        uint8_t volume();

        void setVolume(uint8_t value);

        inline void play(uint32_t sampleRate, Callback cb) { hal::audio::play(sampleRate, cb); }

        void play(DecoderStream * stream);

        inline void recordMic(uint32_t sampleRate, Callback cb) { hal::audio::recordMic(sampleRate, cb); }

        inline void recordLineIn(uint32_t sampleRate, Callback cb) { hal::audio::recordLineIn(sampleRate, cb); }

        inline void pause() { hal::audio::pause(); }

        inline void resume() { hal::audio::resume(); }

        inline void stop() { hal::audio::stop(); }

        inline bool isPlaying() { return hal::audio::isPlaying(); }

        inline bool isRecording() { return hal::audio::isRecording(); }

        inline bool isPaused() { return hal::audio::isPaused(); }

        /** Converts mono buffer into stereo one, duplicating all samples in it in place.
         
            Takes audio buffer and number of mono samples stored in it and expands the mono samples to stereo ones, duplicating each mono sample int two stereo samples in place. The caller must ensure the buffer is large enough to hold the resulting stereo samples. 
        */
        inline void convertToStereo(int16_t * buffer, uint32_t numSamples) {
            for (; numSamples > 0; --numSamples) {
                int16_t x = buffer[numSamples - 1];
                buffer[numSamples * 2 - 2] = x;
                buffer[numSamples * 2 - 1] = x;
            }
        }

    } // namespace rckid::audio


    namespace pim {
    
        uint32_t remainingBudget();

        uint32_t updateBudget(int32_t value);

    } // namespace rckid::pim


} // namespace rckid

/** Following are potentially accelerated hardware primitives. 
 
    Their default implementations are provided by default, but can be overriden by platform-specific implementations for better performance. See platform details for more information. 
 */
extern "C" {
    void memset8(uint8_t * buffer, uint8_t value, uint32_t size);
    void memset16(uint16_t * buffer, uint16_t value, uint32_t size);
    void memset32(uint32_t * buffer, uint32_t value, uint32_t size);
}