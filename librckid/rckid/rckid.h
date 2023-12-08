#pragma once

#include <pico.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/vreg.h>
#include <hardware/i2c.h>
#include <pico/binary_info.h>


#include "ST7789.h"

#include "utils.h"

/** RCKid SDK
 */
namespace rckid {

    using Display = ST7789;

    /** Initializes the basic I/O operations. 
     
        This must be the first function of the SDK being called. 
     */
    void initializeIO(); 

    /** Initializes audio output mic input. 
     
        Should be the second function called unless own audio driver is being used. 
     */
    void initializeAudio();

    /** Initializes the display using the specified pixel format. 
     
        See the actual implementations for the supported pixel formats below. Each initializer should first initialize the display itself, then enter the continuous mode and finally load the appropriate pio driver.  
     */
    template<typename DISPLAY_CONFIG> 
    void initializeDisplay();

    inline void cpu_overclock(unsigned hz) {
        set_sys_clock_khz(hz / 1000, true);
    }

    inline void cpu_overvolt() {
        vreg_set_voltage(VREG_VOLTAGE_1_20);
	    sleep_ms(10);
    }

    inline void cpu_overclock_max() {
        vreg_set_voltage(VREG_VOLTAGE_1_20);
	    sleep_ms(10);
        set_sys_clock_khz(250000, true);
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




    /*
    template<>
    inline void initializeDisplay<DisplayRGBA>() {
        ST7789::initialize();
        ST7789::setColorMode(ST7789::ColorMode::RGB666);
        ST7789::setColumnRange(0, 239);
        ST7789::setRowRange(0, 319);
        ST7789::enterContinuousMode();
        ST7789::loadPIODriver(ST7789_rgba_program, ST7789_rgba_program_init);
        ST7789::startPIODriver();
    }
    */

    /*
    template<>
    inline void initializeDisplay<ColorPicosystem>(int width, int height) {
        uint16_t left = (320 - width) / 2;
        uint16_t top = (240 - height) / 2;
        ST7789::initialize();
        ST7789::setColorMode(ST7789::ColorMode::RGB666);
        ST7789::setColumnRange(top, top + height - 1);
        ST7789::setRowRange(left, left + width - 1);
        ST7789::enterContinuousMode();
        ST7789::loadPIODriver(ST7789_rgba_program, ST7789_rgba_program_init);
        ST7789::startPIODriver();
    }
    */

} // namespace rckid