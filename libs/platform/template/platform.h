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

    static  void outputHigh(Pin p) { write(p, true); setAsOutput(p); write(p, true);  }
    static  void outputLow(Pin p) { write(p, false); setAsOutput(p); write(p, false); }
    static  void outputFloat(Pin p) { setAsInput(p); }

    static void high(Pin p) { write(p, true); }
    static void low(Pin p) { write(p, false); }

}; 

class i2c {
    static bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize);
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
}

