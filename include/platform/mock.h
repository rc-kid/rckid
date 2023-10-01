#pragma once
#include <cstdint>
#include <cstring>
#include <thread>
#include <chrono>

namespace platform {

    class cpu {
    public:
        static void delayUs(unsigned value) {
            std::this_thread::sleep_for(std::chrono::microseconds(value));        
        }

        static void delayMs(unsigned value) {
            std::this_thread::sleep_for(std::chrono::milliseconds(value));        
        }

        static void sleep() {}

    }; // cpu

    class wdt {
    public:
        static void enable() {}
        static void disable() {}
        static void reset() {}
    }; // wdt

    class gpio {
    public:
        using Pin = uint16_t;
        static constexpr Pin UNUSED = 0xffff;

        enum class Edge {
            Rising, 
            Falling, 
            Both
        }; 

        static void initialize() {}

        static void output(Pin pin) {}

        static void input(Pin pin) {}

        static void inputPullup(Pin pin) {}

        static void high(Pin pin) {}

        static void low(Pin pin) {}

        static bool read(Pin pin) { return false; }

        static void attachInterrupt(Pin pin, Edge edge, void (*handler)()) {}

    }; // gpio

    class i2c {
    public:

        static bool initializeMaster() { return true; }

        static void initializeSlave(uint8_t address_) {}

        static bool transmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
            return false;
        }

    }; // i2c

    class spi {
    public:

        using Device = unsigned;

        static bool initialize() { return true; }

        static void begin(Device device) {}

        static void end(Device device) {}

        static uint8_t transfer(uint8_t value) { return 0; }

        static size_t transfer(uint8_t const * tx, uint8_t * rx, size_t numBytes) { return 0; }

        static void send(uint8_t const * data, size_t size) {}

        static void receive(uint8_t * data, size_t size) {}
    }; // spi

} // namespace platform
