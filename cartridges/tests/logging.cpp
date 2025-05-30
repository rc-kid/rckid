#include <rckid/rckid.h>

using namespace rckid;

/** Simple device test that just prints increased number every second to the serial to usb debug port.
 
    This should be the first test to run as it ensures at least some observability of the device is working. Aside from the one second logging the GPIO pin 15 (available at the cartridge header) is toggled every second as the most basic check of a running program.
 */
int main() {
    initialize();
    uint32_t i = 0;
    while (true) {
        if (i % 2 == 0)
            gpio::outputHigh(15);
        else
            gpio::outputLow(15);
        LOG(LL_ERROR, "Tick " << (i++));
        uint32_t next = uptimeUs() + 1000000;
        while (uptimeUs() < next) {
            yield();
        }
    }
}


