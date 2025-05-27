#include <rckid/rckid.h>

using namespace rckid;

void listI2CDevices() {
    debugWrite() << "I2C probe:\n";
    uint32_t n = 0;
    for (uint8_t i = 0; i < 127; ++i) {
        if (i2c::isPresent(i)) {
            debugWrite() << "    " << hex(i) << '\n';
            ++n;
        }
    }
    debugWrite() << "Found " << n << " devices\n";
}


/** Tests various hardware features. 
 */
int main() {
    initialize();
    uint32_t i = 0;
    while (true) {
        if (i % 2 == 0)
            gpio::outputHigh(15);
        else
            gpio::outputLow(15);
        LOG(LL_ERROR, "TickX " << (i++));
        //listI2CDevices();        
        uint32_t next = uptimeUs() + 10000000;
        while (uptimeUs() < next) {
            yield();
        }
    }

    /*
    listI2CDevices();
    while (true) {
        tick();
    }
        */
}
