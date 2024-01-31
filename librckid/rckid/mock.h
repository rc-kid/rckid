#pragma once
#if (defined LIBRCKID_MOCK)

#include <random>

#define __force_inline __always_inline
using PIO = void *;

constexpr int i2c0 = 0;
constexpr int i2c1 = 1;

inline void i2c_write_blocking(int i2c, uint8_t addr, uint8_t const * cmd, size_t len, bool nostop) {
    // TODO I2C LOG
}

inline uint32_t get_rand_32() {
    return std::rand();
}



#endif
