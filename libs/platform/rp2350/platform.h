#pragma once

#include <cstdint>
#include <utility>

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

#define PLATFORM_RP2350
#define PLATFORM_LITTLE_ENDIAN

#include "../definitions.h"
#include "../overload.h"

class cpu {
public:

    static void delayUs(unsigned value) {
        sleep_us(value);  
    }

    static void delayMs(unsigned value) {
        sleep_ms(value);  
    }

    FORCE_INLINE(static void nop() __attribute__((always_inline)) {
        __asm__ volatile ("nop");
    })

    // platform extras

    static size_t clockSpeed() { return clockSpeedkHz_ * 1000; } 

    static void overclock(unsigned hz=200000000, bool overvolt = false) {
        if (overvolt) {
            vreg_set_voltage(VREG_VOLTAGE_1_20);
            sleep_ms(10);
        } else {
            // TODO non-overvolt                
        }
        clockSpeedkHz_ = hz / 1000;
        set_sys_clock_khz(clockSpeedkHz_, true);
    }

    class DisableInterruptsGuard {
    public:
        DisableInterruptsGuard() {
            savedIrqState_ = save_and_disable_interrupts();
        }

        ~DisableInterruptsGuard() {
            restore_interrupts(savedIrqState_);
        }

        DisableInterruptsGuard(DisableInterruptsGuard const &) = delete;
        DisableInterruptsGuard & operator=(DisableInterruptsGuard const &) = delete;
        
    private:
        unsigned savedIrqState_;

    }; // cpu::DisableInterruptsGuard

private:

    static inline unsigned clockSpeedkHz_ = 125000;

}; // cpu

class gpio {
public:

    using Pin = uint16_t;

    static constexpr Pin UNUSED = 0xffff;

    static void setAsOutput(Pin pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
    }

    static void setAsInput(Pin pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
    }

    static void setAsInputPullUp(Pin pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    static void setAsInputPullDown(Pin pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_down(pin);
    }

    static void write(Pin pin, bool value) { gpio_put(pin, value); }

    static bool read(Pin pin) { return gpio_get(pin); }

    #include "../common/gpio_common.h"

    /* TODO do we need this?
    static void initialize() {
        //stdio_init_all();
    }
    */

}; // gpio

class i2c {
public:

    static bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
        if (wsize != 0) {
            int x = i2c_write_blocking(i2c0, address, wb, wsize, rsize != 0);
            if (x != wsize)
                return false;
        }
        if (rsize != 0) {
            int x = i2c_read_blocking(i2c0, address, rb, rsize, false);
            if (x != rsize)
                return false;
        }
        return true;
    }

    // platform extras

    static void initializeMaster(unsigned sda, unsigned scl, unsigned baudrate = 400000) {
        i2c_init(i2c0, baudrate);
        gpio_set_function(sda, GPIO_FUNC_I2C);
        gpio_set_function(scl, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(sda, scl, GPIO_FUNC_I2C));    
    }

    #include "../common/i2c_common.h"

}; // i2c

class spi {
public:

    using Device = gpio::Pin;

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

    #include "../common/spi_common.h"
}; // spi

/** Convenience function for setting PIO speed. 
 */
inline void pio_sm_set_clock_speed(PIO pio, unsigned sm, unsigned hz) {
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
