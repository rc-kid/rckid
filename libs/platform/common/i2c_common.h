    static inline bool isPresent(uint8_t address) {
        uint8_t x_;
        return i2c::masterTransmit(address, nullptr, 0, & x_, 1);
    }

    static inline int masterWrite(uint8_t address, uint8_t * data, uint8_t size) {
        return i2c::masterTransmit(address, data, size, nullptr, 0);
    }

    static int masterRead(uint8_t address, uint8_t * buffer, uint8_t size) {
        return i2c::masterTransmit(address, nullptr, 0, buffer, size);
    }
    
    template<typename T, platform::Endian DEVICE_ENDIAN = platform::Endian::Little>
    static void write(uint8_t address, T data);

    static inline void write(uint8_t address, uint8_t * data, uint8_t size) {
        i2c::masterTransmit(address, data, size, nullptr, 0);
    }

    template<typename T, platform::Endian DEVICE_ENDIAN = platform::Endian::Little>
    static T read(uint8_t address); 

    static inline uint8_t read(uint8_t address, uint8_t * buffer, uint8_t size) {
        return i2c::masterTransmit(address, buffer, size, nullptr, 0) ? size : 0;
    }

    template<typename T, platform::Endian DEVICE_ENDIAN = platform::Endian::Little>
    static void writeRegister(uint8_t address, uint8_t reg, T value);

    template<typename T, platform::Endian DEVICE_ENDIAN = platform::Endian::Little>
    static T readRegister(uint8_t address, uint8_t reg);

    /** Reads dynamically sized buffer starting at given register. 
     */
    static void readRegister(uint8_t address, uint8_t reg, uint8_t * buffer, size_t size) {
        i2c::masterTransmit(address, & reg, 1, buffer, size);
    }


    class Device {
    public:
        uint8_t const address;
    protected:
        Device(uint8_t address): address{address} {}
    }; // i2c::Device

}; // i2c

template<>
inline void i2c::write<uint8_t, platform::Endian::Little>(uint8_t address, uint8_t data) {
    i2c::masterTransmit(address, & data, 1, nullptr, 0);
}
template<>
inline void i2c::write<uint8_t, platform::Endian::Big>(uint8_t address, uint8_t data) {
    i2c::masterTransmit(address, & data, 1, nullptr, 0);
}

template<>
inline void i2c::write<uint16_t, platform::Endian::Little>(uint8_t address, uint16_t data) {
#if (defined PLATFORM_LITTLE_ENDIAN)        
    i2c::masterTransmit(address, reinterpret_cast<uint8_t *>(& data), 2, nullptr, 0);
#else
    #error "unimplemented"
#endif
}   

template<>
inline void i2c::write<uint16_t, platform::Endian::Big>(uint8_t address, uint16_t data) {
    uint8_t d[] = { static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data & 0xff) };
    i2c::masterTransmit(address, d, 2, nullptr, 0);
}   

template<>
inline uint8_t i2c::read<uint8_t, platform::Endian::Little>(uint8_t address) {
    uint8_t result = 0;
    i2c::masterTransmit(address, nullptr, 0, & result, 1);
    return result;
}

template<>
inline uint8_t i2c::read<uint8_t, platform::Endian::Big>(uint8_t address) {
    uint8_t result = 0;
    i2c::masterTransmit(address, nullptr, 0, & result, 1);
    return result;
}

template<>
inline uint16_t i2c::read<uint16_t, platform::Endian::Little>(uint8_t address) {
#if (defined PLATFORM_LITTLE_ENDIAN)
    uint16_t result = 0;
    i2c::masterTransmit(address, nullptr, 0, reinterpret_cast<uint8_t *>(result), 2);
    return result;
#else 
    #error "unimplemented"
#endif
}

template<>
inline uint16_t i2c::read<uint16_t, platform::Endian::Big>(uint8_t address) {
    uint8_t result[2];
    i2c::masterTransmit(address, nullptr, 0, result, 2);
    return (result[0] << 8) | result[1];
}

template<>
inline void i2c::writeRegister<uint8_t, platform::Endian::Little>(uint8_t address, uint8_t reg, uint8_t value) {
    uint8_t buf[] = { reg, value };
    i2c::masterTransmit(address, buf, sizeof(buf), nullptr, 0);
}

template<>
inline void i2c::writeRegister<uint8_t, platform::Endian::Big>(uint8_t address, uint8_t reg, uint8_t value) {
    uint8_t buf[] = { reg, value };
    i2c::masterTransmit(address, buf, sizeof(buf), nullptr, 0);
}

template<>
inline void i2c::writeRegister<uint16_t, platform::Endian::Little>(uint8_t address, uint8_t reg, uint16_t value) {
    uint8_t buf[] = { reg, static_cast<uint8_t>(value  & 0xff), static_cast<uint8_t>((value >> 8) & 0xff)};
    i2c::masterTransmit(address, buf, sizeof(buf), nullptr, 0);
}

template<>
inline void i2c::writeRegister<uint16_t, platform::Endian::Big>(uint8_t address, uint8_t reg, uint16_t value) {
    uint8_t buf[] = { reg, static_cast<uint8_t>((value >> 8) & 0xff), static_cast<uint8_t>(value & 0xff)};
    i2c::masterTransmit(address, buf, sizeof(buf), nullptr, 0);
}

template<>
inline uint8_t i2c::readRegister<uint8_t, platform::Endian::Little>(uint8_t address, uint8_t reg) {
    uint8_t result = 0;
    i2c::masterTransmit(address, & reg, 1, & result, 1);
    return result;
}

template<>
inline uint8_t i2c::readRegister<uint8_t, platform::Endian::Big>(uint8_t address, uint8_t reg) {
    uint8_t result = 0;
    i2c::masterTransmit(address, & reg, 1, & result, 1);
    return result;
}

template<>
inline uint16_t i2c::readRegister<uint16_t, platform::Endian::Little>(uint8_t address, uint8_t reg) {
#if (defined PLATFORM_LITTLE_ENDIAN)
    uint16_t result = 0;
    i2c::masterTransmit(address, & reg, 1, reinterpret_cast<uint8_t*>(& result), 2);
    return result;
#else
    #error "unimplemented"
#endif
}

template<>
inline uint16_t i2c::readRegister<uint16_t, platform::Endian::Big>(uint8_t address, uint8_t reg) {
    uint8_t result[2];
    i2c::masterTransmit(address, & reg, 1, result, 2);
    return (result[0] << 8) | result[1];
}

// open an extra anonymous namespace for the last } in the platform.h (we are no longer in the class definition)
namespace {