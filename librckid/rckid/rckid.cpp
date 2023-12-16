#include "common/config.h"

#include "rckid.h"

#include "ST7789.h"

namespace rckid {

    size_t clockSpeed_ = 125000000;

    void initialize() {
        // initialize the display
        ST7789::initialize();
        // initialize the I2C bus
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  
        // TODO detect and initialize the standard peripherals
        // TODO serial if necessary
    }

    void cpuOverclock(unsigned hz, bool overvolt) {
        if (overvolt) {
            vreg_set_voltage(VREG_VOLTAGE_1_20);
            sleep_ms(10);
        } else {
            // TODO non-overvolt                
        }
        clockSpeed_ = hz;
        set_sys_clock_khz(hz / 1000, true);
    }


    // TODO super dumb nanosecond-like delay. Should be changed to take into account the actual cpu clock speed etc
    void sleep_ns(uint32_t ns) {
        //ns = ns * 2;
        while (ns >= 8) 
          ns -= 8;
    }




    void Device::tick() {
        lastState_ = state_;
        i2c_read_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t *)& state_, sizeof(State), false);
    }
} // namespace rckid