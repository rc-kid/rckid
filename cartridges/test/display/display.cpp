#include "common/config.h"
#include "rckid/rckid.h"
#include "rckid/ST7789.h"

using namespace rckid;
using namespace platform;

/** A very simple debug program that just keeps cycling through red, green and blue, reinitializing the screen each time. 

    Mostly useful for checking the display connection is stable and works all right.  
 */
void rckid_main() {
    gpio::outputHigh(GPIO21);
    while (true) {
        ST7789::reset();
        ST7789::fill(ColorRGB{255, 0, 0});
        gpio::outputLow(GPIO21);
        cpu::delayMs(200);
        gpio::outputHigh(GPIO21);
        ST7789::reset();
        ST7789::fill(ColorRGB{0, 255, 0});
        gpio::outputLow(GPIO21);
        cpu::delayMs(200);
        gpio::outputHigh(GPIO21);
        ST7789::reset();
        ST7789::fill(ColorRGB{0, 0, 255});
        gpio::outputLow(GPIO21);
        cpu::delayMs(200);
        gpio::outputHigh(GPIO21);
    }
}