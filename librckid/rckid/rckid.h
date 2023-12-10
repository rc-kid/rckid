#pragma once

#include <functional>

#include <pico.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/vreg.h>
#include <hardware/i2c.h>
#include <pico/binary_info.h>

#include "common/config.h"

/** RCKid SDK
 */
namespace rckid {

    /** Initializes the basic I/O operations. 
     
        This must be the first function of the SDK being called. 
     */
    void initializeIO(); 

    /** \name CPU Control 
     */
    //@{

    size_t cpuClockSpeed();

    void cpuOverclock(unsigned hz);

    void cpuOvervolt();

    void cpuOverclockMax();

    void sleep_ns(uint32_t ns);

    //@}

    /** \name Utility functions 
     */
    //@{

    inline uint16_t swapBytes(uint16_t x) { return static_cast<uint16_t>((x & 0xff) << 8 | (x >> 8)); }    

    //@}
} // namespace rckid

inline uint8_t operator "" _u8(unsigned long long value) { return static_cast<uint8_t>(value); }
inline uint16_t operator "" _u16(unsigned long long value) { return static_cast<uint16_t>(value); }
inline uint32_t operator "" _u32(unsigned long long value) { return static_cast<uint32_t>(value); }
inline uint64_t operator "" _u64(unsigned long long value) { return static_cast<uint64_t>(value); }

inline void pio_set_clock_speed(PIO pio, unsigned sm, unsigned hz) {
    uint kHz = hz / 1000;
    uint clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS); // [kHz]
    uint clkdiv = (clk / kHz);
    uint clkfrac = (clk - (clkdiv * kHz)) * 256 / kHz;
    pio_sm_set_clkdiv_int_frac(pio, sm, clkdiv & 0xffff, clkfrac & 0xff);
} 
