#error "Do not include this file, it only exists to provide a blueprint for necessary minimal platform interface."

#include <cstdint>
#include "../definitions.h"

class cpu {
public:
    static void delayUs(unsigned value);

    static void delayMs(unsigned value);

    static void nop();
};

class gpio {
public:
    using Pin = unsigned;

    constexpr Pin UNUSED = 0x0;

    static void setAsOutput(Pin pin);

    static void setAsInput(Pin pin);

    static void setAsInputPullup(Pin pin);

    static void setAsOutput(Pin pin);

    static void write(Pin pin, bool value);

    static bool read(Pin pin);

    #include "../common/gpio_common.h"

}; 

class i2c {
    static bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize);

    #include "../common/i2c_common.h"
}; 

class spi {
    using Device = gpio::Pin;

    static void begin(Device device) {
        gpio::write(device, false);
    }

    static void end(Device device) {
        gpio::write(device, true);
    }

    static uint8_t transfer(uint8_t value);

    #include "../common/spi_common.h"
}; // spi

