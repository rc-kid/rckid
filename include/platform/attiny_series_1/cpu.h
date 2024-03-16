#pragma once

class cpu {
public:

    static void delayUs(uint16_t value) {
        _delay_us(value);
    }

    static void delayMs(uint16_t value) {
        _delay_ms(value);
    }

    static void sleep() {
#if (defined ARCH_AVR_MEGATINY)
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_cpu();
#endif
    }

    static void reset() {
#if (defined ARCH_AVR_MEGATINY)
        _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
#endif
    }

}; // cpu
