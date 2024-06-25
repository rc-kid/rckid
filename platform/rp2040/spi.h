#pragma once 

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
