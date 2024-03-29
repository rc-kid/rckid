/** Convenience function for setting PIO speed. 
 */
inline void pio_set_clock_speed(PIO pio, unsigned sm, unsigned hz) {
    uint kHz = hz / 1000;
    uint clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS); // [kHz]
    uint clkdiv = (clk / kHz);
    uint clkfrac = (clk - (clkdiv * kHz)) * 256 / kHz;
    pio_sm_set_clkdiv_int_frac(pio, sm, clkdiv & 0xffff, clkfrac & 0xff);
}
