#pragma once

class cpu {
public:

    static void delayUs(unsigned value) {
        sleep_us(value);  
    }

    static void delayMs(unsigned value) {
        sleep_ms(value);  
    }

    static void sleep() {
        // TODO TODO TODO 
    }

    static size_t clockSpeed() { return clockSpeedkHz_ * 1000; } 

    static void overclock(unsigned hz=250000000, bool overvolt = true) {
        if (overvolt) {
            vreg_set_voltage(VREG_VOLTAGE_1_20);
            sleep_ms(10);
        } else {
            // TODO non-overvolt                
        }
        clockSpeedkHz_ = hz / 1000;
        set_sys_clock_khz(clockSpeedkHz_, true);
    }

    static void nop() __attribute__((always_inline)) {
        __asm__ volatile ("nop");
    }

private:

    static inline unsigned clockSpeedkHz_ = 125000000;

}; // cpu

