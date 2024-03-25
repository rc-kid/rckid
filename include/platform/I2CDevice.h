#pragma once

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
            return i2c::masterTransmit(address, nullptr, 0, nullptr, 0);
        }

        template<typename T, Endian DEVICE_ENDIAN = Endian::Little>
        void write(T data);

        void write(uint8_t * data, uint8_t size) {
            i2c::masterTransmit(address, data, size, nullptr, 0);
        }

        template<typename T, Endian DEVICE_ENDIAN = Endian::Little>
        T read(); 

        uint8_t read(uint8_t * buffer, uint8_t size) {
            return i2c::masterTransmit(address, buffer, size, nullptr, 0) ? size : 0;
        }

        template<typename T, Endian DEVICE_ENDIAN = Endian::Little>
        void writeRegister(uint8_t reg, T value);

        template<typename T, Endian DEVICE_ENDIAN = Endian::Little>
        T readRegister(uint8_t reg);

    }; // i2c::Device

    template<>
    inline void I2CDevice::write<uint8_t, Endian::Little>(uint8_t data) {
        i2c::masterTransmit(address, & data, 1, nullptr, 0);
    }
    template<>
    inline void I2CDevice::write<uint8_t, Endian::Big>(uint8_t data) {
        i2c::masterTransmit(address, & data, 1, nullptr, 0);
    }

    template<>
    inline void I2CDevice::write<uint16_t, Endian::Little>(uint16_t data) {
#if (defined ARCH_LITTLE_ENDIAN)        
        i2c::masterTransmit(address, reinterpret_cast<uint8_t *>(& data), 2, nullptr, 0);
#else
        #error "unimplemented"
#endif
    }   

    template<>
    inline void I2CDevice::write<uint16_t, Endian::Big>(uint16_t data) {
        uint8_t d[] = { static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data & 0xff) };
        i2c::masterTransmit(address, d, 2, nullptr, 0);
    }   

    template<>
    inline uint8_t I2CDevice::read<uint8_t, Endian::Little>() {
        uint8_t result = 0;
        i2c::masterTransmit(address, nullptr, 0, & result, 1);
        return result;
    }

    template<>
    inline uint8_t I2CDevice::read<uint8_t, Endian::Big>() {
        uint8_t result = 0;
        i2c::masterTransmit(address, nullptr, 0, & result, 1);
        return result;
    }

    template<>
    inline uint16_t I2CDevice::read<uint16_t, Endian::Little>() {
    #if (defined ARCH_LITTLE_ENDIAN)
        uint16_t result = 0;
        i2c::masterTransmit(address, nullptr, 0, reinterpret_cast<uint8_t *>(result), 2);
        return result;
    #else 
        #error "unimplemented"
    #endif
    }

    template<>
    inline uint16_t I2CDevice::read<uint16_t, Endian::Big>() {
        uint8_t result[2];
        i2c::masterTransmit(address, nullptr, 0, result, 2);
        return (result[0] << 8) | result[1];
    }

    template<>
    inline void I2CDevice::writeRegister<uint8_t, Endian::Little>(uint8_t reg, uint8_t value) {
        uint8_t buf[] = { reg, value };
        i2c::masterTransmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline void I2CDevice::writeRegister<uint8_t, Endian::Big>(uint8_t reg, uint8_t value) {
        uint8_t buf[] = { reg, value };
        i2c::masterTransmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline void I2CDevice::writeRegister<uint16_t, Endian::Little>(uint8_t reg, uint16_t value) {
        uint8_t buf[] = { reg, static_cast<uint8_t>(value  & 0xff), static_cast<uint8_t>((value >> 8) & 0xff)};
        i2c::masterTransmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline void I2CDevice::writeRegister<uint16_t, Endian::Big>(uint8_t reg, uint16_t value) {
        uint8_t buf[] = { reg, static_cast<uint8_t>((value >> 8) & 0xff), static_cast<uint8_t>(value & 0xff)};
        i2c::masterTransmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline uint8_t I2CDevice::readRegister<uint8_t, Endian::Little>(uint8_t reg) {
        uint8_t result = 0;
        i2c::masterTransmit(address, & reg, 1, & result, 1);
        return result;
    }

    template<>
    inline uint8_t I2CDevice::readRegister<uint8_t, Endian::Big>(uint8_t reg) {
        uint8_t result = 0;
        i2c::masterTransmit(address, & reg, 1, & result, 1);
        return result;
    }

    template<>
    inline uint16_t I2CDevice::readRegister<uint16_t, Endian::Little>(uint8_t reg) {
#if (defined ARCH_LITTLE_ENDIAN)
        uint16_t result = 0;
        i2c::masterTransmit(address, & reg, 1, reinterpret_cast<uint8_t*>(& result), 2);
        return result;
#else
        #error "unimplemented"
#endif
    }

    template<>
    inline uint16_t I2CDevice::readRegister<uint16_t, Endian::Big>(uint8_t reg) {
        uint8_t result[2];
        i2c::masterTransmit(address, & reg, 1, result, 2);
        return (result[0] << 8) | result[1];
    }

} // namespace platform