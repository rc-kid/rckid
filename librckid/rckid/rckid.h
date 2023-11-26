#pragma once

#include <pico.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/vreg.h>
#include <hardware/i2c.h>
#include <pico/binary_info.h>

/** RCKid SDK
 */
namespace rckid {

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

    inline void cpu_overclock(unsigned hz) {
        set_sys_clock_khz(hz / 1000, true);
    }

    inline void cpu_overvolt() {
        vreg_set_voltage(VREG_VOLTAGE_1_20);
	    sleep_ms(10);
    }

    /*
      #ifndef NO_OVERCLOCK
      // Apply a modest overvolt, default is 1.10v.
      // this is required for a stable 250MHz on some RP2040s
      vreg_set_voltage(VREG_VOLTAGE_1_20);
	    sleep_ms(10);
      // overclock the rp2040 to 250mhz
      set_sys_clock_khz(250000, true);
    #endif
    */



} // namespace rckid