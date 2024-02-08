#pragma once
#if (defined LIBRCKID_MOCK)

#include <random>
#include <raylib.h>

#define __force_inline __always_inline
using PIO = void *;
using dma_channel_config = void *;

constexpr int i2c0 = 0;
constexpr int i2c1 = 1;

constexpr int GPIO_OUT = 0;
constexpr int GPIO_IN = 1;

inline void i2c_write_blocking(int i2c, uint8_t addr, uint8_t const * cmd, size_t len, bool nostop) {
    // TODO I2C LOG
}

inline uint32_t get_rand_32() {
    return std::rand();
}

inline uint32_t time_us_32() {
    // TODO
    return 0;
}

inline void gpio_put(int, int) {}
inline void gpio_init(int) {}
inline void gpio_set_dir(int, int) {}




#endif
