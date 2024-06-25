#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>


#define ARCH_ARDUINO
#define ARCH_LITTLE_ENDIAN

class cpu {
public:

    static void delayUs(uint16_t value) {
        // TODO
    }

    static void delayMs(uint16_t value) {
        delay(value);
    }

    static void sleep() {
        // TODO
    }

    static void reset() {
        // TODO
    }

}; // cpu

namespace gpio {

    using Pin = int;
    
    constexpr Pin UNUSED = -1;

    inline void initialize() {}

    inline void setAsOutput(Pin pin) { pinMode(pin, OUTPUT); }

    inline void setAsInput(Pin pin) { pinMode(pin, INPUT); }

    inline void setAsInputPullup(Pin pin) { pinMode(pin, INPUT_PULLUP); }

    inline void write(Pin pin, bool value) { digitalWrite(pin, value ? HIGH : LOW); }

    inline bool read(Pin pin) { return digitalRead(pin); }
}; // gpio

namespace i2c {

    inline void initializeMaster() {
        Wire.begin();
        Wire.setClock(400000);
    }

    inline void initializeSlave(uint8_t address) {
        // TODO
    }

    inline bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
        if (wsize > 0) {
            Wire.beginTransmission(address);
            Wire.write(wb, wsize);
            Wire.endTransmission(rsize == 0); 
        }
        if (rsize > 0) {
            if (Wire.requestFrom(address, rsize) == rsize) {
                Wire.readBytes(rb, rsize);
            } else {
                Wire.flush();
                return false;
            }
        }
        return true;
    }
}; // i2c

namespace spi {

    using Device = gpio::Pin;

    inline void initialize() {
        SPI.begin();
    }

    inline void begin(Device device) {
        gpio::write(device, false);
        SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    }

    inline void end(Device device) {
        gpio::write(device, true);
        SPI.endTransaction();
    }

    inline uint8_t transfer(uint8_t value) {
        return SPI.transfer(value);
    }

    inline size_t transfer(uint8_t const * tx, uint8_t * rx, size_t numBytes) { 
        for (size_t i = 0; i < numBytes; ++i)
            *(rx++) = transfer(*(tx++));
        return numBytes;
    }

    inline void send(uint8_t const * data, size_t numBytes) {
        for (size_t i = 0; i < numBytes; ++i)
            transfer(*(data++));
    }

    inline void receive(uint8_t * data, size_t numBytes) {
        for (size_t i = 0; i < numBytes; ++i)
            *(data++) = transfer(0);
    }

}; // spi

#include "../common.h"
