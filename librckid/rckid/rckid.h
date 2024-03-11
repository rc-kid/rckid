#pragma once

#include <malloc.h>
#include <csetjmp>
#include <cstdint>
#include <memory.h>

#include <functional>

#if (! defined LIBRCKID_MOCK)
    #include <pico.h>
    #include <pico/time.h>
    #include <pico/stdlib.h>
    #include <hardware/pio.h>
    #include <hardware/clocks.h>
    #include <hardware/vreg.h>
    #include <hardware/i2c.h>
    #include <hardware/gpio.h>
    #include <hardware/dma.h>
    #include <hardware/uart.h>
    #include <pico/binary_info.h>
    #include <pico/rand.h>
#else // LIBRCKID_MOCK
    #include "mock.h"
#endif 

#include "common/config.h"
#include "common/state.h"
#include "common/commands.h"

#include "writer.h"
#include "sensors.h"

/** VRAM beginning and end symbols. 
 */
extern uint8_t __vram_start__, __vram_end__;

inline uint8_t operator "" _u8(unsigned long long value) { return static_cast<uint8_t>(value); }
inline uint16_t operator "" _u16(unsigned long long value) { return static_cast<uint16_t>(value); }
inline uint32_t operator "" _u32(unsigned long long value) { return static_cast<uint32_t>(value); }
inline uint64_t operator "" _u64(unsigned long long value) { return static_cast<uint64_t>(value); }

#define LOG(...) ::rckid::writeToUSBSerial() << __VA_ARGS__ << "\r\n"
#define DEBUG(...) ::rckid::writeToUSBSerial() << __VA_ARGS__ << "\r\n"
#define ASSERT(...) if (!(__VA_ARGS__)) { FATAL_ERROR(rckid::ASSERTION_ERROR); }
#define UNIMPLEMENTED FATAL_ERROR(::rckid::NOT_IMPLEMENTED_ERROR)
#define UNREACHABLE FATAL_ERROR(::rckid::UNREACHABLE_ERROR)
#define FATAL_ERROR(CODE) ::rckid::Device::fatalError(CODE, __FILE__, __LINE__)

#define CALCULATE_TIME(...) [&](){ uint32_t start__ = time_us_32(); __VA_ARGS__; return static_cast<unsigned>(time_us_32() - start__); }()


/** The main function of the cartridge. 
 
    This function *must* be implemented by each cartridge. Presumably, it will initialize any applications used by the cartridge and start the root app. When entered, all RCKid'd hardware has been properly initialized, but the cooperative nature of the SDK requires explicit peridodic update calls otherwise the hardware won't be responsive. 
*/
void rckid_main();

/** RCKid SDK
 
    
 */
namespace rckid {

    class App;

    constexpr int INTERNAL_ERROR = 1;
    constexpr int ASSERTION_ERROR = 2;
    constexpr int VRAM_OUT_OF_MEMORY = 3;
    constexpr int HEAP_OUT_OF_MEMORY = 4;

    constexpr int NOT_IMPLEMENTED_ERROR = 256;
    constexpr int UNREACHABLE_ERROR = 257;

    /** Yields to the RCKid's device events. 
         
     */
    void yield();

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

    /** \name Power management 
     */
    //@{

    void powerOff();

    bool charging();
    bool dcPower();

    /** Returns the voltage measured by the AVR in Volts x 100. The value can also be used as a battery gauge level. 
     */
    unsigned vcc();

    void setBrightness(uint8_t brightness);

    //@}

    /** \name Memory 
     
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

    /** \name Controls 
        
        RCKid controls comprise of a dpad, buttons A and B, buttons Select and Start, side home button and volume up & down keys. Furthermore, RCKid is equipped with a 3axis accelerometer and gyroscope as well as an ambient and UV light sensor. 

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
    //@}
   

    /** \name Sensors
    */
    //@{

    /** Returns temperature in Celsius x 10 as measured by the AVR chip. 
     */
    unsigned tempAvr();

    unsigned tempAccel();

    //}

    /** \name I2C Communication 
     
        Provides blocking implementation of the basic I2C transactions (check, read, write, reg read and reg write). As the I2C bus is shared between the user application and the RCKid's system, those functions should only be used in the update() method where the system is guaranteed not to use the I2C bus. 

        The I2C bus is available on the cartridge as well and can therefore be used to communicate with various cartridge specific hardware. 
     */
    //@{

    /** Determines if an I2C device responds on the given address. 
     */
    inline bool i2cDevicePresent(uint8_t address) {
        uint8_t x_;
        return i2c_read_blocking(i2c0, address, & x_, 1, false) >= 0;
    }

    /** Reads given number of bytes from the device. Returns the number of bytes received. 
     */
    inline size_t i2cDeviceRead(uint8_t address, uint8_t * buffer, uint8_t numBytes) {
        return i2c_read_blocking(i2c0, address, buffer, numBytes, false);
    }

    /** Writes the given number of bytes to the provided device. Returns the number of bytes successfully sent. 
     
        TODO is this true? 
     */
    inline size_t i2cDeviceWrite(uint8_t address, uint8_t const * buffer, uint8_t numBytes) {
        return i2c_write_blocking(i2c0, address, buffer, numBytes, false);
    }

    /** First writes, then reads immediately (w/o releasing the bus) provided bytes. Returns the number of bytes received.  */
    inline size_t i2cDeviceWriteThenRead(uint8_t address, uint8_t const * wrBuffer, size_t wrSize, uint8_t * rdBuffer, size_t rdSize) {
        i2c_write_blocking(i2c0, address, wrBuffer, wrSize, true);
        return i2c_read_blocking(i2c0, address, rdBuffer, rdSize, false);
    }

    /** Reads 8bit register from device. 
     */
    inline uint8_t i2cRegisterRead8(uint8_t address, uint8_t reg) {
        uint8_t result;
        i2c_write_blocking(i2c0, address, & reg, 1, true);
        i2c_read_blocking(i2c0, address, & result, 1, false);
        return result;
    }

    /** Writes value of given 8bit register. 
     */
    inline bool i2cRegisterWrite8(uint8_t address, uint8_t reg, uint8_t value) {
        uint8_t x[] = { reg, value };
        return i2c_write_blocking(i2c0, address, x, 2, false) >= 0;
    }

    /** Reads given 16 bit register (little endian). 
     */
    inline uint16_t i2cRegisterRead16(uint8_t address, uint8_t reg) {
        uint16_t result;
        i2c_write_blocking(i2c0, address, & reg, 1, true);
        i2c_read_blocking(i2c0, address, reinterpret_cast<uint8_t *>(& result), 2, false);
        return result;
    }

    /** Writes the given 16bit register (little endian). 
     */
    inline bool i2cRegisterWrite16(uint8_t address, uint8_t reg, uint16_t value) {
        uint8_t x[3] = { reg, static_cast<uint8_t>(value & 0xff), static_cast<uint8_t>(value >> 8)};
        return i2c_write_blocking(i2c0, address, x, 3, false) >= 0;
    }

    //@}

    /** \name SPI Communications 
     */
    //@{

    // TODO

    //@}

    /** \name Time Utilities
     */
    //@{

    /** Returns RP2040's uptime in microseconds. 
     */
    inline uint64_t uptime_us() { return to_us_since_boot(get_absolute_time()); }

    inline uint32_t uptime_us_32() { return time_us_32(); }

    /** Returns the current time as kept by the AVR. 
     */
    TinyDate time();

    //@}

    /** \name CPU Control
     */
    //@{
    size_t cpuClockSpeed();

    void sleep_ns(uint32_t ns);
    inline void delay_ms(uint32_t ms) { sleep_ms(ms); }

    void cpuOverclock(unsigned hz = 250000000, bool overvolt = true);
    //@}

    /** Encapsulates the state of the device and its basic peripherals. 
     
        A static class is used to maintain encapsulation while providing inlinable, fast implementations of the friend functions from the RCKid's API described above. 
     */
    class Device {
    public:

        /** Throws a fatal error. 
         
            Fatal error stops the execution of current app and immediately enters the BSOD, displaying the diagnostic information. To throw a fatal error, the macro FATAL_ERROR should be used instead of calling the function directly as the macro automatically inserts the location information where appropriate.
         */
        static __force_inline void fatalError(int code, char const * file, int line) { 
            fatalErrorFile_ = file;
            fatalErrorLine_ = line;
            longjmp(fatalError_, code); 
        }

        static __force_inline void fatalError(int code) { fatalError(code, nullptr, 0); }
  
    private:
        friend class App;
        friend class Audio;

        /** Pointer to the end of allocated VRAM. 
         */
        static inline uint32_t * vramNext_ = 0;

        static inline uint32_t ticks_ = 0;

        static inline size_t clockSpeed_ = 125000000;

        static inline State state_;
        static inline State lastState_;

        static inline BMI160::State accelState_;

        static inline uint16_t lightALS_ = 0;
        static inline uint16_t lightUV_ = 0;
       
        template<typename T>
        static void sendCommand(T const & cmd) {
            /// TODO: ensure T is a command
            i2c_write_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t const *) & cmd, sizeof(T), false);
        }    

        static void initialize();

        /** Initializes the RCKid. 
         
            Starts the chip and ist subsystems, I2C communication with the AVR and other peripherals and the display. Sets up any necessary structures and memory and then calls the rckid_main function which is never expected to return. 
        */
        friend void start();

        /** Updates the device by talking to all common peripherals, etc. 
         
            For each tick we need to get the following:

            - AVR status - 6 bytes for the buttons, info and config 
            - Sensors

            Each can be programmed by 
         
         */
        static void tick();


        // basic functions
        friend void yield();
        friend void start(App && app);


        // power management
        friend void powerOff();
        friend bool charging() { return state_.status.charging(); }
        friend bool dcPower() { return state_.status.dcPower(); }
        friend unsigned vcc() { return state_.info.vcc(); }

        friend void setBrightness(uint8_t brightness) {
            Device::sendCommand(cmd::SetBrightness(brightness));
        }


        // memory
        friend size_t freeHeap();
        friend size_t freeVRAM();
        friend void resetVRAM();
        friend void * allocateVRAM(size_t numBytes) {
            if (numBytes % 4 != 0)
                numBytes += 4 - (numBytes % 4);
            if (numBytes > freeVRAM())
                FATAL_ERROR(VRAM_OUT_OF_MEMORY);
            void * result = static_cast<void*>(Device::vramPtr_);
            // it's ok to disable the array bounds check here as we have checked the size already above
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
            Device::vramPtr_ += numBytes;
#pragma GCC diagnostic pop            
            return result;
        }
        static inline uint8_t * vramPtr_ = nullptr;

        // controls
        friend bool down(Btn b) { return state_.status.down(b); }
        friend bool pressed(Btn b) { return state_.status.down(b) && ! lastState_.status.down(b); }
        friend bool released(Btn b) { return !state_.status.down(b) && lastState_.status.down(b); }
        friend int16_t accelX() { return accelState_.accelX; }
        friend int16_t accelY() { return accelState_.accelY; }
        friend int16_t accelZ() { return accelState_.accelZ; }
        friend int16_t gyroX() { return accelState_.gyroX; }
        friend int16_t gyroY() { return accelState_.gyroY; }
        friend int16_t gyroZ() { return accelState_.gyroZ; }
        friend uint16_t lightAmbient() { return lightALS_; }
        friend uint16_t lightUV() { return lightUV_; }

        // sensors
        friend unsigned tempAvr() { return state_.info.temp(); }

        // time utilities
        friend TinyDate time() { return state_.time; }

        friend void sleep_ns(uint32_t ns);
        friend size_t cpuClockSpeed() { return clockSpeed_; }
        friend void cpuOverclock(unsigned hz, bool overvolt);

        static void BSOD(int code);

        static inline jmp_buf fatalError_;
        static inline int fatalErrorLine_ = 0;
        static inline char const * fatalErrorFile_ = nullptr;

    }; 

#if (!defined LIBRCKID_MOCK)
    /* When not in mock mode, we have all we need in the header file and can inline the VRAM functions for maximum performance.*/
    inline size_t freeVRAM() { return &__vram_end__ - Device::vramPtr_; }
    inline void resetVRAM() { Device::vramPtr_ = &__vram_start__; }
    inline bool isVRAMPtr(void * ptr) { return (ptr >= static_cast<void*>(& __vram_start__)) && (ptr < static_cast<void*>(& __vram_end__)); }
#endif

    /** Various performance metrics. 
     */
    class Stats {
    public:

        static unsigned fps() { return fps_; }
        static unsigned systemUs() { return systemUs_; }
        static unsigned updateUs() { return updateUs_; }
        static unsigned drawUs() { return drawUs_; }
        static unsigned frameUs() { return frameUs_; }
        static unsigned idleUs() { return frameUs_ - systemUs_ - updateUs_ - drawUs_; }
        static unsigned idlePct() { return idleUs() * 100 / frameUs_; }

        static unsigned lastUpdateUs() { return updateUs_; }
        static unsigned lastUpdateWaitUs() { return updateWaitUs_; }
        static unsigned lastVSyncWaitUs() { return vsyncWaitUs_; }

    private:

        friend class ST7789;
        friend class App;

        static inline unsigned fps_;
        static inline unsigned fpsCounter_;
        static inline unsigned systemUs_;
        static inline unsigned updateUs_;
        static inline unsigned drawUs_;
        static inline unsigned frameUs_;

        static inline uint64_t nextFpsTick_;


        static inline unsigned displayUpdateUs_ = 0;
        static inline unsigned updateWaitUs_ = 0;
        static inline unsigned vsyncWaitUs_ = 0;
        static inline uint64_t updateStart_ = 0;

    }; // rckid::Stats

    /** \name Utility functions and classes
     */
    //@{

    inline uint16_t swapBytes(uint16_t x) { return static_cast<uint16_t>((x & 0xff) << 8 | (x >> 8)); }   

    /** Assumes that the given pointer is aligned to 4 bytes, which is required for speedy memory accesses. We have to roll our own because the std::assume_align is only available from C++20. 
     */
    template<typename T, typename W>
    T assumeAligned(W x) {
        return reinterpret_cast<T>(__builtin_assume_aligned(x, 4));
    }

    /** Interpolation modes - see interpolate() for mode details. 
     
        NOTE: Not to be confused with the RP2040's interpolator HW unit. 
    */
    enum class Interpolation {
        Linear, 
        Sin, 
        Cos, 
    }; // rckid::Interpolation

    /// Sin table for very imprecise integer only sin calculations. 
    static constexpr uint8_t SinTable[] = { 
        0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 
        30, 33, 36, 39, 42, 45, 48, 50, 53, 56,
        58, 61, 63, 66, 68, 70, 72, 75, 77, 79,
        80, 82, 84, 86, 87, 89, 90, 91, 92, 94,
        95, 96, 96, 97, 98, 98, 99, 99, 99, 100
    };

    /** Interpolates value between start and end (both inclusive) using the given promille value and interpolation algorithm. 
     
        - Interpolation::Linear will provide linear increase from start to end wrt promille
        - Interpolation::Sin will ajust the interpolation by sin function, i.e. fast start, slow middle, fast end
        - Interpolation::Cos uses the cos function to adjust the value, i.e. show start, fast middle and slow end, which corresponds to the natural accelerate - brake movement of things 
    */
    template<typename T> 
    inline T interpolate(T start, T end, int promille, Interpolation i = Interpolation::Linear) {
        if (end < start)
            promille *= -1;
        switch (i) {
            default: // a bit of defensive programming
            case Interpolation::Linear:
                return (end - start) * promille / 1000 + start;
            case Interpolation::Sin:
                if (promille <= 500)
                    return static_cast<T>((end - start) * SinTable[(promille + 5) / 10] / 200 + start);
                else
                    return static_cast<T>((end - start) * (200 - SinTable[sizeof(SinTable) - 1 - (promille - 500 + 5) / 10]) / 200 + start);
            case Interpolation::Cos:
                if (promille <= 505)
                    return static_cast<T>((end - start) * (100 - SinTable[sizeof(SinTable) - 1 - ((promille + 5) / 10)]) / 200 + start);
                else
                    return static_cast<T>((end - start) * (100 + SinTable[(promille - 510 + 5) / 10]) / 200 + start);
        }
    }

    /** A simple timer utility with [us] precision. 
     
        The timer starts when initialized, total and lap times can be retrieved. Particularly useful for measuring various performance metrics. 
    */
    class Timer {
    public:
        Timer():
            start_{uptime_us_32()},
            lapStart_{start_} {
        }

        /** Returns total time in [us]. */
        unsigned total() const { return uptime_us_32() - start_; }

        /** Returns current lap time in [us]. */
        unsigned lap() const { return uptime_us_32() - lapStart_; }

        /** Retrurns the length of current lap and starts a new one in [us]. */
        unsigned newLap() {
            unsigned t = uptime_us_32();
            unsigned result = t - lapStart_;
            lapStart_ = t;
            return result;
        }

    private:
        
        unsigned start_;
        unsigned lapStart_;
    }; 

    //@}
} // namespace rckid

/** Convenience function for setting PIO speed. 
 */
void pio_set_clock_speed(PIO pio, unsigned sm, unsigned hz);
