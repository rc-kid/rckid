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
    listI2CDevices();
    while (true) {
        tick();
    }
}
