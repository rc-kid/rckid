#pragma once

#include <Arduino.h>

#define ARCH_ARDUINO
#define ARCH_LITTLE_ENDIAN

class cpu {
public:

    static void delayUs(uint16_t value) {
        // TODO
    }

    static void delayMs(uint16_t value) {
        // TODO
    }

    static void sleep() {
        // TODO
    }

    static void reset() {
        // TODO
    }

}; // cpu

class gpio {
public:
    using Pin = int;
    static constexpr Pin UNUSED = -1;

    static void initialize() {}

    static void setAsOutput(Pin pin) {
        pinMode(pin, OUTPUT);
    }

    static void setAsInput(Pin pin) {
        pinMode(pin, INPUT);
    }

    static void setAsInputPullup(Pin pin) {
        pinMode(pin, INPUT_PULLUP);
    }

    static void write(Pin pin, bool value) {
        digitalWrite(pin, value ? HIGH : LOW);
    }

    static bool read(Pin pin) {
        return digitalRead(pin);
    }
}; // gpio

class i2c {
public:

    static void initializeMaster() {
        Wire.begin();
        Wire.setClock(400000);
    }

    static void initializeSlave(uint8_t address) {
        // TODO
    }

    static bool masterTransmit(uint8_t address, uint8_t const * wb, uint8_t wsize, uint8_t * rb, uint8_t rsize) {
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

class spi {
public:

    using Device = gpio::Pin;

    static void initialize() {
        SPI.begin();
    }

    static void begin(Device device) {
        gpio::write(device, false);
        SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    }

    static void end(Device device) {
        gpio::write(device, true);
        SPI.endTransaction();
    }

    static uint8_t transfer(uint8_t value) {
        return SPI.transfer(value);
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
