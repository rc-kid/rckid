#pragma once

#include <cstdint>

/** Literal constructors for unsigned types with varying sizes. 
 */
inline uint8_t operator "" _u8(unsigned long long value) { return static_cast<uint8_t>(value); }
inline uint16_t operator "" _u16(unsigned long long value) { return static_cast<uint16_t>(value); }
inline uint32_t operator "" _u32(unsigned long long value) { return static_cast<uint32_t>(value); }
inline uint64_t operator "" _u64(unsigned long long value) { return static_cast<uint64_t>(value); }

namespace platform {

    /** Endiannes of the platform. 
     */
    enum class Endian {
        Little, 
        Big, 
    }; 

    /** Assumes that the given pointer is aligned to 4 bytes, which is required for speedy memory accesses. We have to roll our own because the std::assume_align is only available from C++20. 
     */
    template<typename T, typename W>
    T assumeAligned(W x) {
        return reinterpret_cast<T>(__builtin_assume_aligned(x, 4));
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

    /** Swaps the high and low nibble of an uint16_t. 
     */
    inline uint16_t swapBytes(uint16_t x) {
        return (x >> 8) | (x << 8);
    }

    template<typename T> 
    void writeMask(T & where, T mask, bool value) {
        where = value ? (where | mask) : (where & ~mask);
    }


    /** Calculates number of 1s in given 32bit number. 
     */
    inline unsigned popCount(uint32_t x) {
        constexpr uint32_t m1 = 0x55555555;
        constexpr uint32_t m2 = 0x33333333;
        constexpr uint32_t m4 = 0x0f0f0f0f;
        constexpr uint32_t h01 = 0x01010101;
        x -= (x >> 1) & m1;             
        x = (x & m2) + ((x >> 2) & m2);  
        x = (x + (x >> 4)) & m4;         
        return (x * h01) >> 24;   
    }

} // namespace platform 

/** GPIO helper functions. 
 */
namespace gpio {
    inline void outputHigh(Pin p) { write(p, true); setAsOutput(p); write(p, true);  }
    inline void outputLow(Pin p) { write(p, false); setAsOutput(p); write(p, false); }
    inline void outputFloat(Pin p) { setAsInput(p); }

    inline void high(Pin p) { write(p, true); }
    inline void low(Pin p) { write(p, false); }
} // namespace gpio

/** I2C helper functions. 
 */
namespace i2c {

    class Device {
    public:
        const uint8_t address;

    protected:

        Device(uint8_t address): address{address} { }

    }; 

    inline bool isPresent(uint8_t address) {
        uint8_t x_;
        return masterTransmit(address, nullptr, 0, & x_, 1);
    }

    template<typename T, platform::Endian DEVICE_ENDIAN = platform::Endian::Little>
    void write(uint8_t address, T data);

    inline void write(uint8_t address, uint8_t * data, uint8_t size) {
        masterTransmit(address, data, size, nullptr, 0);
    }

    template<typename T, platform::Endian DEVICE_ENDIAN = platform::Endian::Little>
    T read(uint8_t address); 

    inline uint8_t read(uint8_t address, uint8_t * buffer, uint8_t size) {
        return masterTransmit(address, buffer, size, nullptr, 0) ? size : 0;
    }

    template<typename T, platform::Endian DEVICE_ENDIAN = platform::Endian::Little>
    void writeRegister(uint8_t address, uint8_t reg, T value);

    template<typename T, platform::Endian DEVICE_ENDIAN = platform::Endian::Little>
    T readRegister(uint8_t address, uint8_t reg);

    /** Reads dynamically sized buffer starting at given register. 
     */
    inline void readRegister(uint8_t address, uint8_t reg, uint8_t * buffer, size_t size) {
        masterTransmit(address, & reg, 1, buffer, size);
    }

    template<>
    inline void write<uint8_t, platform::Endian::Little>(uint8_t address, uint8_t data) {
        masterTransmit(address, & data, 1, nullptr, 0);
    }
    template<>
    inline void write<uint8_t, platform::Endian::Big>(uint8_t address, uint8_t data) {
        masterTransmit(address, & data, 1, nullptr, 0);
    }

    template<>
    inline void write<uint16_t, platform::Endian::Little>(uint8_t address, uint16_t data) {
#if (defined ARCH_LITTLE_ENDIAN)        
        masterTransmit(address, reinterpret_cast<uint8_t *>(& data), 2, nullptr, 0);
#else
        #error "unimplemented"
#endif
    }   

    template<>
    inline void write<uint16_t, platform::Endian::Big>(uint8_t address, uint16_t data) {
        uint8_t d[] = { static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data & 0xff) };
        masterTransmit(address, d, 2, nullptr, 0);
    }   

    template<>
    inline uint8_t read<uint8_t, platform::Endian::Little>(uint8_t address) {
        uint8_t result = 0;
        masterTransmit(address, nullptr, 0, & result, 1);
        return result;
    }

    template<>
    inline uint8_t read<uint8_t, platform::Endian::Big>(uint8_t address) {
        uint8_t result = 0;
        masterTransmit(address, nullptr, 0, & result, 1);
        return result;
    }

    template<>
    inline uint16_t read<uint16_t, platform::Endian::Little>(uint8_t address) {
    #if (defined ARCH_LITTLE_ENDIAN)
        uint16_t result = 0;
        masterTransmit(address, nullptr, 0, reinterpret_cast<uint8_t *>(result), 2);
        return result;
    #else 
        #error "unimplemented"
    #endif
    }

    template<>
    inline uint16_t read<uint16_t, platform::Endian::Big>(uint8_t address) {
        uint8_t result[2];
        masterTransmit(address, nullptr, 0, result, 2);
        return (result[0] << 8) | result[1];
    }

    template<>
    inline void writeRegister<uint8_t, platform::Endian::Little>(uint8_t address, uint8_t reg, uint8_t value) {
        uint8_t buf[] = { reg, value };
        masterTransmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline void writeRegister<uint8_t, platform::Endian::Big>(uint8_t address, uint8_t reg, uint8_t value) {
        uint8_t buf[] = { reg, value };
        masterTransmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline void writeRegister<uint16_t, platform::Endian::Little>(uint8_t address, uint8_t reg, uint16_t value) {
        uint8_t buf[] = { reg, static_cast<uint8_t>(value  & 0xff), static_cast<uint8_t>((value >> 8) & 0xff)};
        masterTransmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline void writeRegister<uint16_t, platform::Endian::Big>(uint8_t address, uint8_t reg, uint16_t value) {
        uint8_t buf[] = { reg, static_cast<uint8_t>((value >> 8) & 0xff), static_cast<uint8_t>(value & 0xff)};
        masterTransmit(address, buf, sizeof(buf), nullptr, 0);
    }

    template<>
    inline uint8_t readRegister<uint8_t, platform::Endian::Little>(uint8_t address, uint8_t reg) {
        uint8_t result = 0;
        masterTransmit(address, & reg, 1, & result, 1);
        return result;
    }

    template<>
    inline uint8_t readRegister<uint8_t, platform::Endian::Big>(uint8_t address, uint8_t reg) {
        uint8_t result = 0;
        masterTransmit(address, & reg, 1, & result, 1);
        return result;
    }

    template<>
    inline uint16_t readRegister<uint16_t, platform::Endian::Little>(uint8_t address, uint8_t reg) {
#if (defined ARCH_LITTLE_ENDIAN)
        uint16_t result = 0;
        masterTransmit(address, & reg, 1, reinterpret_cast<uint8_t*>(& result), 2);
        return result;
#else
        #error "unimplemented"
#endif
    }

    template<>
    inline uint16_t readRegister<uint16_t, platform::Endian::Big>(uint8_t address, uint8_t reg) {
        uint8_t result[2];
        masterTransmit(address, & reg, 1, result, 2);
        return (result[0] << 8) | result[1];
    }

} // namespace i2c
