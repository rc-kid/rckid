#pragma once

#include <malloc.h>
#include <csetjmp>
#include <memory.h>
#include <functional>

#include <platform.h>

#include "common/config.h"
#include "common/state.h"
#include "common/commands.h"

#include "writer.h"
#include "sensors.h"

/** VRAM beginning and end symbols. 
 */
extern uint8_t __vram_start__, __vram_end__;


#define LOG(...) ::rckid::writeToUSBSerial() << __VA_ARGS__ << "\r\n"
#define DEBUG(...) ::rckid::writeToUSBSerial() << __VA_ARGS__ << "\r\n"
#define ASSERT(...) if (!(__VA_ARGS__)) { FATAL_ERROR(rckid::ASSERTION_ERROR); }
#define UNIMPLEMENTED FATAL_ERROR(::rckid::NOT_IMPLEMENTED_ERROR)
#define UNREACHABLE FATAL_ERROR(::rckid::UNREACHABLE_ERROR)
#define FATAL_ERROR(CODE) ::rckid::Device::fatalError(CODE, __FILE__, __LINE__)

#define CALCULATE_TIME(...) [&](){ uint32_t start__ = time_us_32(); __VA_ARGS__; return static_cast<unsigned>(time_us_32() - start__); }()

/** RCKid SDK
 


    I2C Communication

    The platform's blocking I2C functions defauklt to i2c0, which is the one used by rckid as well. Since the I2C bus is shared between system (talking to system sensors and the AVR in charge of power & controls) and whatever sensors might be on a particular cartridge, the I2C communications should only ever be used in the update() function during which no activity on the I2C bus is guaranteed by the system. 

    SPI Communication

    TODO use platform's functions

 
 */
namespace rckid {

    /** \name Cartridge pins. 
     */
    //@{
    constexpr gpio::Pin GPIO14 = gpio::Pin{14};
    constexpr gpio::Pin GPIO15 = gpio::Pin{15};
    constexpr gpio::Pin GPIO16 = gpio::Pin{16};
    constexpr gpio::Pin GPIO17 = gpio::Pin{17};
    constexpr gpio::Pin GPIO18 = gpio::Pin{18};
    constexpr gpio::Pin GPIO19 = gpio::Pin{19};
    constexpr gpio::Pin GPIO20 = gpio::Pin{20};
    constexpr gpio::Pin GPIO21 = gpio::Pin{21};
    //@}

    /** Buttons. 
    */
    enum class Btn {
        Left, 
        Right,
        Up, 
        Down, 
        A, 
        B, 
        Select, 
        Start,
        Home, 
        VolumeUp, 
        VolumeDown,
    }; // rckid::Btn

    constexpr int INTERNAL_ERROR = 1;
    constexpr int ASSERTION_ERROR = 2;
    constexpr int VRAM_OUT_OF_MEMORY = 3;
    constexpr int HEAP_OUT_OF_MEMORY = 4;

    constexpr int NOT_IMPLEMENTED_ERROR = 256;
    constexpr int UNREACHABLE_ERROR = 257;


    /** Yields to the RCKid's device events. 
     */
    void yield();


    /** \name Device management 
     */
    //@{


    void powerOff();

    /** Returns true if the device is currently charging the li-ion battery (must be in DC power mode). False when the charge has completed while still DC power attached, or running from batteries. 
     */
    bool charging();

    /** Returns true if the device is running from DC power, false when power from battery. 
     */
    bool dcPower();

    /** Returns the VCC voltage in Volts x 100. 

        The VCC voltage should either be almost identical to the battery voltage if running on batterier, or be close to 5.0 V (value 500) when running on DC power. Note that this voltage is not available on the cartridge, which only supports the converted 3V3 user by the display and the RP2040 chip. 
     */
    unsigned vcc();

    /** Returns the battery voltage in Volts x 100, i.e. full battery should read 420. 
     */
    unsigned vBatt(); 

    /** Sets the LCD screen brightness. 
     
        0 for backlight off, 255 for maximum brightness. 
     */
    void setBrightness(uint8_t brightness);

    //@}

    /** \name Controls & Sensors
        
        RCKid controls comprise of a dpad, buttons A and B, buttons Select and Start, side home button and volume up & down keys. Furthermore, RCKid is equipped with a 3axis accelerometer and gyroscope as well as an ambient and UV light sensor. 

        Finally, the AVR and the accelerometer can also measure temperature. 

     */
    //@{

    /** Returns true if the given button is currently down. 
     */
    bool down(Btn b);

    /** Returns true if the given button has been pressed since last frame (i.e. its state went from up to down). The value is stable within one frame. */
    bool pressed(Btn b);

    /** Returns true if the given button has been released since last frame (i.e. its state went from down to up). The value is stable within one frame. */
    bool released(Btn b);

    int16_t accelX(); 
    int16_t accelY();
    int16_t accelZ();
    int16_t gyroX();
    int16_t gyroY();
    int16_t gyroZ();

    uint16_t lightAmbient();
    uint16_t lightUV(); 

    /** Returns temperature in Celsius x 10 as measured by the AVR chip. 
     */
    unsigned tempAvr();

    unsigned tempAccel();

    //}

    /** \name Time Utilities
     */
    //@{

    /** Returns RP2040's uptime in microseconds. 
     */
    //inline uint64_t uptimeUs() { return to_us_since_boot(get_absolute_time()); }

    inline uint32_t uptimeUs() { return time_us_32(); }

    /** Returns the current time as kept by the AVR. 
     */
    TinyDate time();

    //@}

    /** \name Memory management
     
        The two small memory baks (4KB each) are used for core 0 and core 1 stacks, while the rest 4 banks are split between data segment, heap and VRAM. VRAM is a special arena-allocated space designed to store the relatively large graphic buffers (framebuffers, tiles, aux buffers, etc.) that applications require without any fragmentation to the heap. 

        The VRAM is cleaned every time an app goes out of focus and reinitialized when app gains focus and so can also be used for any app-wide objects to conserve heap if not all VRAM is necessary for a framebuffer. 
     */
    //@{

    size_t freeHeap();

    /** Returns the free VRAM memory available for new allocations. 
     */
    size_t freeVRAM();

    /** Resets the VRAM memory, deallocating everything in the VRAM arena. 
     
        Use this function with *extreme* caution as it invalidates all pointers to VRAM memory held by the program.
    */
    void resetVRAM();

    /** Allocates given amount of bytes in VRAM and returns pointer. Will go immediately to BSOD if not enough memory is available in VRAM for the allocation. Although one byte granularity for the size is supported, the result is always aligned to 4 bytes and therefore returned as uint32_t pointer. 
     */
    void * allocateVRAM(size_t numBytes);

    /** Allocates VRAM for given type. 
     */
    template<typename T>
    T * allocateVRAM() { return static_cast<T*>(allocateVRAM(sizeof(T))); }

    /** Returns true if the pointer points to VRAM area, false otherwise. 
     */
    bool isVRAMPtr(void * ptr);

    //@}


    /** \name Debugging Support
     */
    //@{

    /** Returns a writes to the USB virtual COM port.
     */
    Writer writeToUSBSerial();

    /** Serial port interface for RCKid allowing for printf statements and somewhat easier debugging. 
     
        To use the serial port on Raspberry Pi, start minicom with the following arguments:

        minicom -b 115200 -o -D /dev/ttyAMA0
     */
    void enableSerialPort();

    //@}


} // namespace rckid

/** The main function of the cartridge. 
 
    This function *must* be implemented by each cartridge. Presumably, it will initialize any applications used by the cartridge and start the root app. When entered, all RCKid'd hardware has been properly initialized, but the cooperative nature of the SDK requires explicit peridodic update calls otherwise the hardware won't be responsive. 
*/
void rckid_main();


// include the device class that encapsulates the state and provides inline variants of the API functions above
#include "device.h"
