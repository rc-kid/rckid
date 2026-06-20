#include <rckid/rckid.h>
#include <rckid/hal.h>
#include <rckid/graphics/color.h>

using namespace rckid;

void showStatus() {
//    LOG(LL_INFO, "Headphones: " << hal::audio::)
}

/** Very simple test that serial output is working. 
 
    Counts display TE periods (fps) and displays update each second (60fps). Also shows the state of various pins that are RP2350 only as there is no communication with AVR yet in this test.
 */
int main() {
    initialize();
    LOG(LL_INFO, "Init done");
    uint32_t i = 0;
    uint32_t te = 0;
    while (true) {
        while (hal::display::vSync() == false)
            yield();
        while (hal::display::vSync() == true)
            yield();
        if (++te == 60) {
            LOG(LL_INFO, "60 te: " << ++i);
            te = 0;
        }
    }
}