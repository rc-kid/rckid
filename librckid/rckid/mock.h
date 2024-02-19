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

inline int i2c_write_blocking(int i2c, uint8_t addr, uint8_t const * cmd, size_t len, bool nostop) {
    return 0;
}

inline int i2c_read_blocking(int size, uint8_t addr, uint8_t * data, size_t len, bool nonstop) {
    return 0;
}

inline uint32_t get_rand_32() {
    return std::rand();
}

inline uint32_t time_us_32() { return static_cast<uint32_t>(GetTime() * 1000000); }
inline uint64_t to_us_since_boot(double t) { return static_cast<uint64_t>(t * 1000000); }
inline double get_absolute_time() { return GetTime(); }
inline void sleep_ms(size_t ms) { }

inline void gpio_put(int, int) {}
inline void gpio_init(int) {}
inline void gpio_set_dir(int, int) {}




#endif
