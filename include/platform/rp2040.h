#pragma once
#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <pico/binary_info.h>

namespace platform {

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

    }; // cpu

    class wdt {
    public:
        static void enable() {
            // TODO TODO TODO
        }
        static void disable() {
            // TODO TODO TODO
        }
        static void reset() {
            // TODO TODO TODO
        }
    }; // wdt

    class gpio {
    public:
        using Pin = uint16_t;
        static constexpr Pin UNUSED = 0xffff;

        static void initialize() {
            //stdio_init_all();
        }

        static void output(Pin pin) {
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_OUT);
        }

        static void input(Pin pin) {
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_IN);
        }

        static void inputPullup(Pin pin) {
            gpio_set_dir(pin, GPIO_IN);
            gpio_pull_up(pin);
        }

        static void high(Pin pin) {
            gpio_put(pin, true);
        }

        static void low(Pin pin) {
            gpio_put(pin, false);
        }

        static bool read(Pin pin) {
            return gpio_get(pin);
        }
    }; // gpio

    class i2c {
    public:

        static void initializeMaster(unsigned sda, unsigned scl, unsigned baudrate = 400000) {
            i2c_init(i2c1, baudrate);
            gpio_set_function(sda, GPIO_FUNC_I2C);
            gpio_set_function(scl, GPIO_FUNC_I2C);
            // Make the I2C pins available to picotool
            bi_decl(bi_2pins_with_func(sda, scl, GPIO_FUNC_I2C));    
        }

        //static void initializeSlave(uint8_t address_) {}

        static bool transmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
            if (wsize != 0)
                i2c_write_blocking(i2c1, address, wb, wsize, rsize != 0);
            if (rsize != 0)
                i2c_read_blocking(i2c1, address, rb, rsize, false);
            return true;
        }
    }; // i2c

    class spi {
    public:

        using Device = gpio::Pin;

        static void initialize(unsigned miso, unsigned mosi, unsigned sck) {
            spi_init(spi0, 5000000); // init spi0 at 0.5Mhz
            gpio_set_function(miso, GPIO_FUNC_SPI);
            gpio_set_function(mosi, GPIO_FUNC_SPI);
            gpio_set_function(sck, GPIO_FUNC_SPI);
            // Make the SPI pins available to picotool
            bi_decl(bi_3pins_with_func(miso, mosi, sck, GPIO_FUNC_SPI));        
        }

        static void begin(Device device) {
    //        asm volatile("nop \n nop \n nop");
            gpio::low(device);
    //        asm volatile("nop \n nop \n nop");
        }

        static void end(Device device) {
            gpio::high(device);
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

    class pio {
    public:
        static void set_clock_speed(PIO pio, uint sm, uint hz) {
            uint clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000; // [Hz]
            uint clkdiv = (clk / hz);
            uint clkfrac = (clk - (clkdiv * hz)) * 256 / hz;
            pio_sm_set_clkdiv_int_frac(pio, sm, clkdiv & 0xffff, clkfrac & 0xff);
        }
    }; // pio

} // namespace platform
