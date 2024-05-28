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
    // to mimick RP2040's overclocking
    inline void overclock(unsigned hz = 250000000, bool overvolt = true) {}
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

#include "../common.h"
#include "../fonts.h"
#include "../peripherals/color_strip.h"

/** RP2040 compatibility layer. 
 
 */
//@{

#include <random>

#include <chrono>
//#include <raylib.h>

#define __force_inline __always_inline

using PIO = void *;
using dma_channel_config = void *;

constexpr int i2c0 = 0;
constexpr int i2c1 = 1;

constexpr int GPIO_OUT = 0;
constexpr int GPIO_IN = 1;

inline int i2c_write_blocking(int i2c, uint8_t addr, uint8_t const * cmd, size_t len, bool nostop) {
    return 0;
}

inline int i2c_read_blocking(int size, uint8_t addr, uint8_t * data, size_t len, bool nonstop) {
    return 0;
}

inline uint32_t get_rand_32() {
    return std::rand();
}

inline uint32_t time_us_32() { 
    using namespace std::chrono;
    static auto first = steady_clock::now();
    return static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - first).count()); 
}

inline uint64_t to_us_since_boot(double t) { return static_cast<uint64_t>(t * 1000000); }
//inline double get_absolute_time() { return GetTime(); }
inline void sleep_ms(size_t ms) { }

inline void gpio_put(int, int) {}
inline void gpio_init(int) {}
inline void gpio_set_dir(int, int) {}

//@}

