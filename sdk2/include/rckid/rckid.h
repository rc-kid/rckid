#pragma once

#include <platform.h>
#include <platform/tinydate.h>

#include <rckid/error.h>
#include <rckid/log.h>
#include <rckid/hal.h>


#define RCKID_DEFAULT_ANIMATION_DURATION_MS 500

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

    /** Returns writer for debugging purposes. 
     
        This is a direct wrapper over the hal::device::debugWrite() function that allows writing dbeug information to preferred debug outputs, such as USB serial adpater, or direct serial output via cartridge pins.
     */
    using hal::device::debugWrite;

    /** Reads characters from debug input.
     
        The call is non-blocking and returns 0 if no data is available. The function is a direct wrapper over the hal::device::debugRead() function.
     */
    using hal::device::debugRead;



    /** Display manipulation.
        
     */
    class display {
    public:
        using RefreshDirection = hal::display::RefreshDirection;
        using Callback = hal::display::Callback;

        static constexpr Coord WIDTH = hal::display::WIDTH;
        static constexpr Coord HEIGHT = hal::display::HEIGHT;
            
        static void waitUpdateDone() {
            while (hal::display::updateActive())
                yield();
        }

        static void waitVSync() {
            while (hal::display::vSync())
                yield();
            while (! hal::display::vSync())
                yield();
        }

        static void enable(Rect rect, RefreshDirection  direction) {
            ASSERT(Rect::WH(WIDTH, HEIGHT).contains(rect));
            waitUpdateDone();
            hal::display::enable(rect, direction);
            rect_ = rect;
            refreshDirection_ = direction;
        }

        static void update(Callback cb) { 
            hal::display::update(cb);
        }

        static uint8_t brightness() { return brightness_; }

        static void setBrightness(uint8_t value) {
            hal::display::setBrightness(value);
            brightness_ = value;
        }
    
    private:
        // state so that we can query display state
        static inline Rect rect_;
        static inline RefreshDirection refreshDirection_;
        static inline uint8_t brightness_ = 128;

    }; // rckid::display


    class pim {
    public:

        static uint32_t remainingBudget() { return budget_; }

        static uint32_t updateBudget(int32_t value) {
            if (value > 0) {
                budget_ += value;
            } else {
                uint32_t absValue = static_cast<uint32_t>(-value);
                if (absValue > budget_)
                    budget_ = 0;
                else
                    budget_ -= absValue;
            }
            return budget_;
        }

    private:

        friend void onSecondTick();

        static inline uint32_t budget_ = 60;

    }; // rckid::pim


} // namespace rckid

/** Following are potentially accelerated hardware primitives. 
 
    Their default implementations are provided by default, but can be overriden by platform-specific implementations for better performance. See platform details for more information. 
 */
extern "C" {
    void memset8(uint8_t * buffer, uint8_t value, uint32_t size);
    void memset16(uint16_t * buffer, uint16_t value, uint32_t size);
    void memset32(uint32_t * buffer, uint32_t value, uint32_t size);
}