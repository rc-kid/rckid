#include <rckid/rckid.h>

#include <rckid/audio/tone.h>

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

    while (true) {
        tick();
        uint64_t next = uptimeUs64() + 1000000;
        while (uptimeUs64() < next) {
            yield();
        }
        LOG(LL_INFO, "Uptime : " << (uptimeUs64() / 1000000) << " ms");
        LOG(LL_INFO, "  VCC: " << powerVcc() << " (usb: " << (powerUsbConnected() ? "Y" : "N") << ", chrg: " << (powerCharging() ? "Y" : "N") << ")");
        LOG(LL_INFO, "  Headphones: " << (audioHeadphones() ? "Y" : "N"));
        rumblerEffect(RumblerEffect::Fail());
    }
}
