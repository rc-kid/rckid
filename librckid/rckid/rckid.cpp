#include "common/config.h"

#include "rckid.h"

namespace rckid {

    size_t clockSpeed_ = 125000000;

    void initializeIO() {
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  

        // TODO detect and initialize the standard peripherals
        // TODO serial if necessary
    }

    size_t cpuClockSpeed() { return clockSpeed_; }

    void cpuOverclock(unsigned hz) {
        clockSpeed_ = hz;
        set_sys_clock_khz(hz / 1000, true);
    }

    void cpuOvervolt() {
        vreg_set_voltage(VREG_VOLTAGE_1_20);
	    sleep_ms(10);
    }

    void cpuOverclockMax() {
        vreg_set_voltage(VREG_VOLTAGE_1_20);
	    sleep_ms(10);
        set_sys_clock_khz(250000, true);
        clockSpeed_ = 250000000;
    }

    // TODO super dumb nanosecond-like delay. Should be changed to take into account the actual cpu clock speed etc
    void sleep_ns(uint32_t ns) {
        while (ns >= 8) 
          ns -= 8;
    }
} // namespace rckid