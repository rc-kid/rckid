#pragma once

#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <hardware/vreg.h>
#include <hardware/dma.h>
#include <hardware/uart.h>
#include <hardware/sync.h>
#include <hardware/pwm.h>
#include <pico.h>
#include <pico/binary_info.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include <pico/rand.h>

#define ARCH_RP2040
#define ARCH_LITTLE_ENDIAN

class cpu {
public:

    static void delayUs(unsigned value) {
        sleep_us(value);  
    }

    static void delayMs(unsigned value) {
        sleep_ms(value);  
    }

    static void sleep() {
        // TODO TODO TODO 
    }

    static size_t clockSpeed() { return clockSpeedkHz_ * 1000; } 

    static void overclock(unsigned hz=200000000, bool overvolt = true) {
        if (overvolt) {
            vreg_set_voltage(VREG_VOLTAGE_1_20);
            sleep_ms(10);
        } else {
            // TODO non-overvolt                
        }
        clockSpeedkHz_ = hz / 1000;
        set_sys_clock_khz(clockSpeedkHz_, true);
    }

    static void nop() __attribute__((always_inline)) {
        __asm__ volatile ("nop");
    }

private:

    static inline unsigned clockSpeedkHz_ = 125000;

}; // cpu

namespace gpio {

    using Pin = uint16_t;

    constexpr Pin UNUSED = 0xffff;

    inline void initialize() {
        //stdio_init_all();
    }

    inline void setAsOutput(Pin pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
    }

    inline void setAsInput(Pin pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
    }

    inline void setAsInputPullup(Pin pin) {
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    inline void setAsInputPullDown(Pin pin) {
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_down(pin);
    }

    inline void write(Pin pin, bool value) { gpio_put(pin, value); }

    inline bool read(Pin pin) { return gpio_get(pin); }
}; // gpio

#pragma once

namespace i2c {

    inline void initializeMaster(unsigned sda, unsigned scl, unsigned baudrate = 400000) {
        i2c_init(i2c0, baudrate);
        gpio_set_function(sda, GPIO_FUNC_I2C);
        gpio_set_function(scl, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(sda, scl, GPIO_FUNC_I2C));    
    }

    //static void initializeSlave(uint8_t address_) {}

    inline bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
        if (wsize != 0)
            i2c_write_blocking(i2c0, address, wb, wsize, rsize != 0);
        if (rsize != 0)
            i2c_read_blocking(i2c0, address, rb, rsize, false);
        return true;
    }

}; // i2c

class spi {
public:

    using Device = gpio::Pin;

    static void initialize(unsigned miso, unsigned mosi, unsigned sck) {
        spi_init(spi0, 500000); // init spi0 at 0.5Mhz
        gpio_set_function(miso, GPIO_FUNC_SPI);
        gpio_set_function(mosi, GPIO_FUNC_SPI);
        gpio_set_function(sck, GPIO_FUNC_SPI);
        // Make the SPI pins available to picotool
        bi_decl(bi_3pins_with_func(miso, mosi, sck, GPIO_FUNC_SPI));        
    }

    static void setSpeed(unsigned speed) {
        spi_init(spi0, speed);
    }

    static void begin(Device device) {
//        asm volatile("nop \n nop \n nop");
        gpio::write(device, false);
//        asm volatile("nop \n nop \n nop");
    }

    static void end(Device device) {
        gpio::write(device, true);
    }

    static uint8_t transfer(uint8_t value) {
        uint8_t result;
        spi_read_blocking(spi0, value, & result, 1);
        return result;
    }

    static size_t transfer(uint8_t const * tx, uint8_t * rx, size_t numBytes) { 
        for (size_t i = 0; i < numBytes; ++i)
            *(rx++) = transfer(*(tx++));
        return numBytes;
    }

    static void send(uint8_t const * data, size_t numBytes) {
        for (size_t i = 0; i < numBytes; ++i)
            transfer(*(data++));
    }

    static void receive(uint8_t * data, size_t numBytes) {
        for (size_t i = 0; i < numBytes; ++i)
            *(data++) = transfer(0);
    }

}; // spi

/** Convenience function for setting PIO speed. 
 */
inline void pio_set_clock_speed(PIO pio, unsigned sm, unsigned hz) {
    uint kHz = hz / 1000;
    uint clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS); // [kHz]
    uint clkdiv = (clk / kHz);
    uint clkfrac = (clk - (clkdiv * kHz)) * 256 / kHz;
    pio_sm_set_clkdiv_int_frac(pio, sm, clkdiv & 0xffff, clkfrac & 0xff);
}

/** Returns true if the given PIO and state machine are enabled, false otherwise. 
 */
inline bool pio_sm_is_enabled(PIO pio, uint sm) {
    return pio->ctrl & (1u << sm);
}

/** Returns trye if the  given PWM is active. 
 */
inline bool pwm_is_enabled(uint slice_num) {
    return (pwm_hw->slice[slice_num].csr) & (1 << PWM_CH0_CSR_EN_LSB);
}

#include "../definitions.h"
#include "../utils.h"
#include "../common.h"

