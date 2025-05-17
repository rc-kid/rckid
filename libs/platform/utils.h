#pragma once

#include <cstdint>

/** Literal constructors for unsigned types with varying sizes. 
 */
inline uint8_t operator "" _u8(unsigned long long value) { return static_cast<uint8_t>(value); }
inline uint16_t operator "" _u16(unsigned long long value) { return static_cast<uint16_t>(value); }
inline uint32_t operator "" _u32(unsigned long long value) { return static_cast<uint32_t>(value); }
inline uint64_t operator "" _u64(unsigned long long value) { return static_cast<uint64_t>(value); }

namespace platform {

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


    inline uint32_t hash(uint8_t const * buffer, uint32_t n) {
        uint32_t r = 0;
        for (uint32_t i = 0; i < n; ++i)
            r = r + (buffer[i] << (i % 24));
        return r + n;
    }

} // namespace platform 
