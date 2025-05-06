#include <rckid/rckid.h>

using namespace rckid;

/** Simple device test that just prints increased number every second to the serial to usb debug port.
 
    This should be the first test to run as it ensures at least some observability of the device is working. 
 */
int main() {
    initialize();
    uint32_t i = 0;
    while (true) {
        LOG(LL_ERROR, "Tick " << (i++));
        uint32_t next = uptimeUs() + 1000000;
        while (uptimeUs() < next) {
            yield();
        }
    }
}