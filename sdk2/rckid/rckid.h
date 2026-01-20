#pragma once

#include <platform.h>
#include <platform/tinydate.h>

/** RCKid SDK

 */


namespace rckid {

    // TODO move this somewhere so that it can be utilized by avr as well without bloating the avr build with defs below? 
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
    void yield();

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

    // TODO rapid fire


    // time

    /** Returns system uptime in microseconds. 
     
        The function is a direct wrapper over hal::time::uptimeUs().
     */
    uint64_t uptimeUs();




    // debugging

    /** Returns writer for debugging purposes. 
     
        This is a direct wrapper over the hal::device::debugWrite() function that allows writing dbeug information to preferred debug outputs, such as USB serial adpater, or direct serial output via cartridge pins.
     */
    Writer debugWrite();

    /** Reads characters from debug input.
     
        The call is non-blocking and returns 0 if no data is available. The function is a direct wrapper over the hal::device::debugRead() function.
     */
    uint8_t debugRead();

} // namespace rckid

/** Following are potentially accelerated hardware primitives. 
 
    Their default implementations are provided by default, but can be overriden by platform-specific implementations for better performance. See platform details for more information. 
 */
extern "C" {
    void memset8(uint8_t * buffer, uint8_t value, uint32_t size);
    void memset16(uint16_t * buffer, uint16_t value, uint32_t size);
    void memset32(uint32_t * buffer, uint32_t value, uint32_t size);
}