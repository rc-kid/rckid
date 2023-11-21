#pragma once

/** RCKid SDK
 */
namespace rckid {

    inline uint16_t swapBytes(uint16_t x) {
        return static_cast<uint16_t>((x & 0xff) << 8 | (x >> 8));
    }    

} // namespace rckid