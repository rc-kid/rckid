#pragma once
#include <stdint.h>
#include <string.h>

/** Determine the main platform architecture. This can either be force specified by the build process, or in some cases automatically detected. Explicit specification takes precedence.
 */

#if (defined ARCH_MOCK)
#elif (defined ARCH_RPI)
#elif (defined ARCH_ARDUINO)
#elif (defined ARCH_RPI2040)
#elif (defined ARDUINO)
    #define ARCH_ARDUINO
#elif (defined PICO_BOARD)
    #define ARCH_RP2040
#else 
    #error "Platform not detected, or specified."
#endif

/** Sub-platforms. These are autodetected. 
 */
#if (defined __AVR_ATtiny1616__)
    #define ARCH_AVR_MEGATINY
    #define ARCH_ATTINY_1616
#elif (defined __AVR_ATtiny1604__)
    #define ARCH_AVR_MEGATINY
    #define ARCH_ATTINY_1604
#elif  (defined __AVR_ATtiny3216__)
    #define ARCH_AVR_MEGATINY
    #define ARCH_ATTINY_3216
#elif (defined __AVR_ATmega8__)
    #define ARCH_AVR_MEGA
#endif

#if (defined ARCH_MOCK)
    #include "mock.h"
#elif (defined ARCH_ARDUINO)
    #include "arduino.h"
#elif (defined ARCH_RP2040)
    #include "rp2040.h"
#elif (defined ARCH_RPI)
    #include "rpi.h"
#endif

namespace platform {

    /** A prototype of I2C device. 
     */
    class I2CDevice {
    public:
        const uint8_t address;
    protected:
        I2CDevice(uint8_t address):
            address{address} {
        }

        bool isPresent() {
            return i2c::transmit(address, nullptr, 0, nullptr, 0);
        }

        template<typename T>
        void write(T data);

        void write(uint8_t * data, uint8_t size) {
            i2c::transmit(address, data, size, nullptr, 0);
        }

        template<typename T>
        T read(); 

        uint8_t read(uint8_t * buffer, uint8_t size) {
            return i2c::transmit(address, buffer, size, nullptr, 0) ? size : 0;
        }

        template<typename T>
        void writeRegister(uint8_t reg, T value);

        template<typename T>
        T readRegister(uint8_t reg);

    }; // i2c::Device

    template<>
    inline void I2CDevice::write<uint8_t>(uint8_t data) {
        i2c::transmit(address, & data, 1, nullptr, 0);
    }

    template<>
    inline void I2CDevice::write<uint16_t>(uint16_t data) {
        i2c::transmit(address, reinterpret_cast<uint8_t *>(& data), 2, nullptr, 0);
    }   

    template<>
    inline uint8_t I2CDevice::read<uint8_t>() {
        uint8_t result = 0;
        i2c::transmit(address, nullptr, 0, & result, 1);
        return result;
    }

    template<>
    inline uint16_t I2CDevice::read<uint16_t>() {
        uint16_t result = 0;
        i2c::transmit(address, nullptr, 0, reinterpret_cast<uint8_t *>(result), 2);
        return result;
    }

    template<>
    inline void I2CDevice::writeRegister<uint8_t>(uint8_t reg, uint8_t value) {
        uint8_t buf[] = { reg, value };
        i2c::transmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline void I2CDevice::writeRegister<uint16_t>(uint8_t reg, uint16_t value) {
        uint8_t buf[] = { reg, static_cast<uint8_t>((value >> 8) & 0xff), static_cast<uint8_t>(value & 0xff)};
        i2c::transmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline uint8_t I2CDevice::readRegister<uint8_t>(uint8_t reg) {
        uint8_t result = 0;
        i2c::transmit(address, & reg, 1, & result, 1);
        return result;
    }

    template<>
    inline uint16_t I2CDevice::readRegister<uint16_t>(uint8_t reg) {
        uint16_t result = 0;
        i2c::transmit(address, & reg, 1, reinterpret_cast<uint8_t*>(& result), 2);
        return result;
    }

    inline uint8_t fromHex(char c) {
        if (c >= '0' && c <= '9')
            return c - '0';
        else if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        else if (c>= 'a' && c <= 'f')
            return c - 'a' + 10;
        else
            return 0;
    }

    inline char toHex(uint8_t value) {
        if (value < 10)
            return '0' + value;
        else if (value < 16)
            return 'a' + value - 10;
        else 
            return '?'; // error
    }


} // namespace platform