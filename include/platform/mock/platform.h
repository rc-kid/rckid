#pragma once

#include <cstdint>
#include <cstring>
#include <thread>
#include <chrono>

#define ARCH_MOCK
#define ARCH_LITTLE_ENDIAN

namespace cpu {

    inline void delayUs(unsigned value) {
        std::this_thread::sleep_for(std::chrono::microseconds(value));        
    }

    inline void delayMs(unsigned value) {
        std::this_thread::sleep_for(std::chrono::milliseconds(value));        
    }

    inline void sleep() {}

    inline size_t clockSpeed() { return 100000000; }
    inline void overclock(unsigned hz, bool overvolt = true) {}
}


namespace gpio {
    using Pin = uint16_t; 

    constexpr Pin UNUSED = 0xffff;

    inline void initialize() {
        //stdio_init_all();
    }

    inline void setAsOutput(Pin pin) {
    }

    inline void setAsInput(Pin pin) {
    }

    inline void setAsInputPullup(Pin pin) {
    }

    inline void write(Pin pin, bool value) { }

    inline bool read(Pin pin) { return false; }
}

namespace i2c {
    inline void initializeMaster(unsigned sda, unsigned scl, unsigned baudrate = 400000) { }

    inline void initializeSlave(uint8_t address_) {}

    inline bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) { 
        return true;
    }
} // i2c

namespace spi {
    using Device = gpio::Pin;

    inline void initialize(unsigned miso, unsigned mosi, unsigned sck) {
    }

    inline void begin(Device device) {
    }

    inline void end(Device device) {
    }

    inline uint8_t transfer(uint8_t value) {
        return 0;
    }

    inline size_t transfer(uint8_t const * tx, uint8_t * rx, size_t numBytes) { 
        return numBytes;
    }

    inline void send(uint8_t const * data, size_t numBytes) {
    }

    inline void receive(uint8_t * data, size_t numBytes) {
    }

} // spi
 

namespace platform {

} // platform

#include "../utils.h"
#include "../I2CDevice.h"
#include "../fonts.h"
#include "../peripherals/color_strip.h"

