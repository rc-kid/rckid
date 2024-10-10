#pragma once

class cpu {
public:

    static void delayUs(uint16_t value) {
        _delay_us(value);
    }

    static void delayMs(uint16_t value) {
        while (value-- != 0) {
            // make sure we reset the wdt before cycles for more robustness
            __asm__ __volatile__ ("wdr"::);
            _delay_ms(1);
        }
    }

    static void sleep() {
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_cpu();
    }

    static void reset() {
        _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
    }

    static void wdtReset() __attribute__((always_inline)) {
        __asm__ __volatile__ ("wdr"::);
    }

}; // cpu
