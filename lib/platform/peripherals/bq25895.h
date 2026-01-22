#pragma once

#include "platform.h"

namespace platform {

    /** BQ25895 
     */
    class BQ25895 : public i2c::Device {
    public:

        BQ25895(uint8_t address): i2c::Device{address} {}

        uint8_t getErrors() {
            return readRegister()
        }


    private:
        static const uint8_t REG00 = 0x00;
        static const uint8_t EN_HIZ = 0x80;
        static const uint8_t EN_ILIM = 0x40;
        static const uint8_t ILIM_MASK = 0x3f;

        static const uint8_t REG01 = 0x01;

        static const uint8_t REG02 = 0x02;
        static const uint8_t REG03 = 0x03;
        static const uint8_t REG04 = 0x04;
        static const uint8_t REG05 = 0x05;



    }; // platform::BQ25895

} // namespace platform