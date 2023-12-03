#pragma once

#include <cstdint>

namespace rckid {

    inline uint8_t operator "" _u8(unsigned long long value) { return static_cast<uint8_t>(value); }
    inline uint16_t operator "" _u16(unsigned long long value) { return static_cast<uint16_t>(value); }

    inline uint16_t swapBytes(uint16_t x) {
        return static_cast<uint16_t>((x & 0xff) << 8 | (x >> 8));
    }    

    // TODO super dumb nanosecond-like delay. Should be changed to take into account the actual cpu clock speed etc
    inline void sleep_ns(uint32_t ns) {
        while (ns >= 8) 
          ns -= 8;
    }

} // namespace rckid