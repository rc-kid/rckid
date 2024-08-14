#pragma once


#include <platform.h>
#include <platform/writer.h>

#include "graphics/color.h"
#include "graphics/geometry.h"

namespace rckid {

    /** Initializes the RCKid console. 
     
        This must be the first SDK function called by the application. 
     */
    void initialize();

    void tick();

    void yield();

    /** Returns debug writer for logging and tracing purposes. 
     
        Depending on the backend, this is either the serial over USB port for the physical devices, or standard output for the fantasy console. 
     */
    Writer debugWrite();

    /** \name Controls & Sensors
        
        RCKid controls comprise of a dpad, buttons A and B, buttons Select and Start, side home button and volume up & down keys. Furthermore, RCKid is equipped with a 3axis accelerometer and gyroscope as well as an ambient and UV light sensor. 
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
        // TODO should we yield here?
        while (displayUpdateActive()) { };
    }

    //@}

    /** \name Memory Management

        RCKid uses a mixed arena-heap model where the heap can be split into different arenas and when arena is exitted, all its memory is freed. This is particularly useful for running apps as everytime an app is executed, new arena is created and when the app is done, the arena is destroyed cleaning up any app-related memory leaks and defragmentation issues.  
     */
    //@{

    /** Returns free heap available to the application. The actual free heap is likely larger if any allocations were freed, but not returned to the heap.      
     */
    uint32_t memoryFreeHeap();

    /** Returns the heap already used. Reverse to the memoryFreeHeap(), this value contains all deallocated, but not yet returned memory as well. 
     */
    uint32_t memoryUsedHeap();

    bool memoryIsOnHeap(void * ptr);

    bool memoryIsInCurrentArena(void * ptr);
 
    /** Enters new memory arena.
     */
    void memoryEnterArena();

    /** Leaves current memory arena. 
     
        Can only be called if arena has previously been entered by memoryEnterArena(). Frees *all* memory of the arena that is being left. 
     */
    void memoryLeaveArena();

    /** Allocates new memory on the heap in current arena. 
     */
    void * malloc(size_t numBytes);
 
    /** Frees previously allocated chunk of memory. Note that the chunk *must* belong to the curren arena. 
     */
    void free(void * ptr);
    //@}

} // namespace rckid
