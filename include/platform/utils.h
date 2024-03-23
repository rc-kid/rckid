#pragma once

namespace platform {

    /** Endiannes of the platform. 
     */
    enum class Endian {
        Little, 
        Big, 
    }; 

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
    inline uint16_t swap(uint16_t x) {
        return (x >> 8) | (x << 8);
    }

}