#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <utility>

#define ARCH_PC
#define PLATFORM_LITTLE_ENDIAN

#include "../definitions.h"
#include "../writer.h"
#include "../overload.h"


#if (! defined PLATFORM_NO_STDSTRING)
#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()
#endif

#if defined(_MSC_VER)
/** Swaps bytes in an uint16_t. GCC builtin that is not available on MSVC. 
 */
inline constexpr uint16_t platform__builtin_bswap16(uint16_t x) {
    return (x >> 8) | ((x & 0xff) << 8);
}

#define __builtin_bswap16 platform__builtin_bswap16

#endif

namespace cpu {

    inline void delayMs(size_t value) {
        std::this_thread::sleep_for(std::chrono::milliseconds(value));
    }

    inline void delayUs(size_t value) {
        std::this_thread::sleep_for(std::chrono::microseconds(value));
    }

    inline void overclock(unsigned) {
        // do nothing
    }

}

namespace gpio {
    using Pin = uint16_t;

    inline void setAsOutput([[maybe_unused]] Pin pin) {
        // TODO
    }

    inline void setAsInput([[maybe_unused]] Pin pin) {
        // TODO
    }

    inline void setAsInputPullup([[maybe_unused]] Pin pin) {
        // TODO
    }

    inline void outputHigh([[maybe_unused]] Pin pin) {
        // TODO
    }

    inline void outputLow([[maybe_unused]] Pin pin) {
        // TODO
    }
}

namespace i2c {

    inline bool masterTransmit([[maybe_unused]] uint8_t address, [[maybe_unused]] uint8_t const * wb, [[maybe_unused]] uint8_t wsize, [[maybe_unused]] uint8_t * rb, [[maybe_unused]] uint8_t rsize) {
        // do nothing
        return false; 
    }

    #include "../common/i2c_common.h"
}

#include "../utils.h"

