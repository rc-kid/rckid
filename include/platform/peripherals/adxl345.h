#pragma once

#include "platform/platform.h"

namespace platform {

    class ADXL345: public I2CDevice {
    public:

        ADXL345(uint8_t address):
            I2CDevice{address} {
        }

        void enable() {
            // set bandwidth rate to 100Hz
            writeRegister<uint8_t>(BW_RATE, 0x0b);
            // set range to 16g, use full resolution
            writeRegister<uint8_t>(DATA_FORMAT, 8 | 3);
            // enable
            writeRegister<uint8_t>(POWER_CTL, 8);
        }

        void disable() {
            writeRegister<uint8_t>(POWER_CTL, 0);
        }
        
        bool isEnabled() {
            return readRegister<uint8_t>(POWER_CTL) | 8;
        }

        uint16_t readX() {
            uint16_t x = readRegister<uint16_t>(DATAX0);
            return x;
        }
            
        uint16_t readY() {
            uint16_t x = readRegister<uint16_t>(DATAY0);
            return x;
        }
            
        uint16_t readZ() {
            uint16_t x = readRegister<uint16_t>(DATAZ0);
            return x;
        }


    private:
        const uint8_t BW_RATE = 0x2c;
        const uint8_t POWER_CTL = 0x2d;
        const uint8_t DATAX0 = 0x32;
        const uint8_t DATAY0 = 0x34;
        const uint8_t DATAZ0 = 0x36;
        const uint8_t DATA_FORMAT = 0x31;
        
        
    };

} // namespace platform
