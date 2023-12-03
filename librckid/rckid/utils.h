#pragma once

#include <cstdint>

#include <hardware/clocks.h>

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

    inline void pio_set_clock_speed(PIO pio, unsigned sm, unsigned hz) {
        uint clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000; // [Hz]
        uint clkdiv = (clk / hz);
        uint clkfrac = (clk - (clkdiv * hz)) * 256 / hz;
        pio_sm_set_clkdiv_int_frac(pio, sm, clkdiv & 0xffff, clkfrac & 0xff);
    } 

} // namespace rckid