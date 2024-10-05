#pragma once

#include <stdint.h>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>

#define ARCH_PC

#include "../definitions.h"

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

#if defined(_MSC_VER)

/** Swaps bytes in an uint16_t. GCC builtin that is not available on MSVC. 
 */
inline uint16_t __builtin_bswap16(uint16_t x) {
    return (x >> 8) | ((x & 0xff) << 8);
}

#endif

namespace cpu {

    inline void delayMs(size_t value) {
        std::this_thread::sleep_for(std::chrono::milliseconds(value));
    }

    inline void delayUs(size_t value) {
        std::this_thread::sleep_for(std::chrono::microseconds(value));
    }
}

#include "../utils.h"

