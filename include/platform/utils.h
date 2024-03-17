#pragma once

namespace platform {

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

}