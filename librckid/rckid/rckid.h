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
#include "ST7789_rgb.pio.h"
#include "ST7789_rgba.pio.h"
#include "color.h"

#include "utils.h"

/** RCKid SDK
 */
namespace rckid {

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
    template<typename PIXEL_FORMAT> 
    void initializeDisplay(int width = 320, int height = 240);

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



    /** Initializes the display for the native RGB 16bit pixels. 
     */
    template<>
    inline void initializeDisplay<ColorRGB>(int width, int height) {
        ST7789::initialize();
        ST7789::enterContinuousMode(width, height);
        ST7789::loadPIODriver(ST7789_rgb_program, ST7789_rgb_program_init);
        ST7789::startPIODriver();
    }

    template<>
    inline void initializeDisplay<ColorRGBA>(int width, int height) {
        ST7789::initialize();
        ST7789::setColorMode(ST7789::ColorMode::RGB666);
        ST7789::enterContinuousMode(width, height);
        ST7789::loadPIODriver(ST7789_rgba_program, ST7789_rgba_program_init);
        ST7789::startPIODriver();
    }

} // namespace rckid