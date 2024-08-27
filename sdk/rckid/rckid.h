#pragma once

#include <platform.h>
#include <platform/writer.h>

#include "graphics/geometry.h"

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

    /** Error enum. 
     
        To be used with the fatalError() function. The following error codes are reserved for the SDK, while any value equal or larger to Error::User can be used by the application itself. 
     */
    enum class Error : uint32_t {
        NoError = 0, 
        Unimplemented = 1,
        Unreachable, 
        Assert, 
        User, 
    }; // rckid::Error

    /** Initializes the RCKid console. 
     
        This must be the first SDK function called by the application. 
     */
    void initialize();

    void tick();

    void yield();

    NORETURN(void fatalError(uint32_t error, uint32_t line = 0, char const * file = nullptr));

    NORETURN(void fatalError(Error error, uint32_t line = 0, char const * file = nullptr));

    /** Returns the system's uptime in microseconds. 
     
        For performance reasons, this uses uint32_t as the result value and as such will overflow every hour & something. The intended purpose of this function is not precise timekeeping, but delta time measurements, so the overflows are fine. 
     */
    uint32_t uptimeUs();

    /** Generates random number in the 32bit unsigned range. 
     */
    uint32_t random();

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
    bool btnDown(Btn b);

    /** Returns true if the given button has been pressed since last frame (i.e. its state went from up to down). The value is stable within one frame. */
    bool btnPressed(Btn b);

    /** Returns true if the given button has been released since last frame (i.e. its state went from down to up). The value is stable within one frame. */
    bool btnReleased(Btn b);

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

    /** \name RGB LEDs
     
        The top plate buttons (DPAD, A, B and Select and Start) each contain one RGB LED underneath that can be used for various light effects. There is also sixth LED in the upper display border that is used for notifications and is not expected to be changed by the application directly. 

        The RGBs support simple effects, such as breathe, rainbow hue, etc.  

        Since the LEDs are neopixels at 5 volts and together consume at least 6mA even if completelt black, it is important to turn them off via ledsOff() function whenever they are not needed. 
     */
    //@{

    /** LED effect specification. 
     */
    class LEDEffect {

    };

    /** Turns all the LEDs off to save power. 
     */
    void ledsOff(); 

    void ledSetEffect(Btn b, LEDEffect const & effect);

    void ledSetEffects(LEDEffect const & dpad, LEDEffect const & a, LEDEffect const & b, LEDEffect const & select, LEDEffect const & start); 

    //@}

    /** \name Rumbler 
     
        A simple rumbler interface that allows playing a rumbler effect that consists of N iterations of rumble with particular strength for a time followed by a pause.
     */
    //@{

    void rumble(uint8_t intensity, uint16_t duration, unsigned repetitions, uint16_t offDuration); 

    inline void rumble(uint8_t intensity, uint16_t duration, unsigned repetitions = 1) {
        rumble(intensity, duration, repetitions, duration);
    }
    //@}

    /** \name SD Card Filesystem access. 
     
        RCKid device contains an SD card that can be used to store various data that can persist between different cartridges, such as music, messages, images, etc. The actual filesystem is handled by the FatFS library where the RCKid SDK only provides the necessary functions that enable the filesystem access. 
     */
    //@{

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

    /** Returns true if there is active memory arena (i.e. when it is safe to call memoryLeaveArena(). 
     */
    bool memoryInsideArena();
 
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

    /** Returns the beginning of the heap. 
     */
    char * heapStart();
    //@}






    /** \name Accelerated functions
     */
    //@{

    void memFill(uint8_t * buffer, uint32_t size, uint8_t value);
    void memFill(uint16_t * buffer, uint32_t size, uint16_t value);
    void memFill(uint32_t * buffer, uint32_t size, uint32_t value);

    //@}

} // namespace rckid
