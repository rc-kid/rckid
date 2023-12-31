#pragma once

#include <functional>

#include <pico.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/vreg.h>
#include <hardware/i2c.h>
#include <hardware/uart.h>
#include <pico/binary_info.h>
#include <pico/rand.h>
//#include <stdio.h>

#include "common/config.h"
#include "common/state.h"
#include "common/commands.h"

inline uint8_t operator "" _u8(unsigned long long value) { return static_cast<uint8_t>(value); }
inline uint16_t operator "" _u16(unsigned long long value) { return static_cast<uint16_t>(value); }
inline uint32_t operator "" _u32(unsigned long long value) { return static_cast<uint32_t>(value); }
inline uint64_t operator "" _u64(unsigned long long value) { return static_cast<uint64_t>(value); }

/** RCKid SDK
 */
namespace rckid {

    /** Initializes the RCKid. 
     
        Starts the chip and ist subsystems, I2C communication with the AVR and other peripherals and the display. This must be the first function called by any RCKid cartridge. 
     */
    void initialize();

    /** Yields to the RCKid's device events. 
     
         
     */
    void yield();

    /** Serial port interface for RCKid allowing for printf statements and somewhat easier debugging. 
     
        To use the serial port on Raspberry Pi, start minicom with the following arguments:

        minicom -b 115200 -o -D /dev/ttyAMA0
     */
    inline void enableSerialPort() {
        stdio_uart_init_full(
            RP_DEBUG_UART, 
            RP_DEBUG_UART_BAUDRATE, 
            RP_DEBUG_UART_TX_PIN, 
            RP_DEBUG_UART_RX_PIN
        );
    }

    /** \name Controls 
        
     */
    //@{

    bool down(Btn b);

    bool pressed(Btn b);

    bool released(Btn b);

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

    /** \name Sensors
    */
    //@{

    /** Returns temperature in Celsius x 10 as measured by the AVR chip. 
     */
    unsigned tempAvr();

    //}

    /** \name Real Time Clock
     */
    //@{

    /** Returns RP2040's uptime in microseconds. 
     */
    inline uint64_t uptime_us() { return to_us_since_boot(get_absolute_time()); }

    /** Returns the current time as kept by the AVR. 
     */
    TinyDate time();

    //@}

    /** \name CPU Control
     */
    //@{
    size_t cpuClockSpeed();

    void sleep_ns(uint32_t ns);

    void cpuOverclock(unsigned hz = 250000000, bool overvolt = true);
    //@}


    /** Encapsulates the state of the device and its basic peripherals. 
     
        A static class is used to maintain encapsulation while providing inlinable, fast implementations of the friend functions from the RCKid's API described above. 
     */
    class Device {
    public:

  
    private:
        friend class BaseApp;
        friend class Audio;

        static inline size_t clockSpeed_ = 125000000;

        static inline State state_;
        static inline State lastState_;

        template<typename T>
        static void sendCommand(T const & cmd) {
            /// TODO: ensure T is a command
            i2c_write_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t const *) & cmd, sizeof(T), false);
        }    

        /** Updates the device by talking to all common peripherals, etc. 
         
            For each tick we need to get the following:

            - AVR status - 6 bytes for the buttons, info and config 
            - Sensors

            Each can be programmed by 
         
         */
        static void tick();

        friend void initialize();

        friend void yield();

        friend bool down(Btn b) { return state_.status.down(b); }
        friend bool pressed(Btn b) { return state_.status.down(b) && ! lastState_.status.down(b); }
        friend bool released(Btn b) { return !state_.status.down(b) && lastState_.status.down(b); }

        friend void powerOff();
        friend bool charging() { return state_.status.charging(); }
        friend bool dcPower() { return state_.status.dcPower(); }
        friend unsigned vcc() { return state_.info.vcc(); }

        friend void setBrightness(uint8_t brightness) {
            Device::sendCommand(cmd::SetBrightness(brightness));
        }

        friend unsigned tempAvr() { return state_.info.temp(); }

        friend TinyDate time() { return state_.time; }

        friend void sleep_ns(uint32_t ns);
        friend size_t cpuClockSpeed() { return clockSpeed_; }
        friend void cpuOverclock(unsigned hz, bool overvolt);

    }; 

    /** \name Utility functions 
     */
    //@{

    inline uint16_t swapBytes(uint16_t x) { return static_cast<uint16_t>((x & 0xff) << 8 | (x >> 8)); }    

    //@}
} // namespace rckid

inline void pio_set_clock_speed(PIO pio, unsigned sm, unsigned hz) {
    uint kHz = hz / 1000;
    uint clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS); // [kHz]
    uint clkdiv = (clk / kHz);
    uint clkfrac = (clk - (clkdiv * kHz)) * 256 / kHz;
    pio_sm_set_clkdiv_int_frac(pio, sm, clkdiv & 0xffff, clkfrac & 0xff);
} 
