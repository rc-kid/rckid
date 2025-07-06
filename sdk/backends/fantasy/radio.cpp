#include <rckid/radio.h>

namespace rckid {
    void Radio::enable(bool value) {
        LOG(LL_INFO, "Radio enable: " << value);
    }

    void Radio::initialize() {
        LOG(LL_INFO, "Radio initialize");
        instance_ = new Radio{};
    }

    void Radio::reset() {
        LOG(LL_INFO, "Radio reset");
    }

    void Radio::sendCommand(uint8_t const *, uint8_t, uint32_t) {
        LOG(LL_INFO, "Radio send command");
    }

    void Radio::getResponse(uint8_t) {
        LOG(LL_INFO, "Radio get response");
    }

    void Radio::irqHandler() { }

    void Radio::processResponse(uint8_t) { } 

} // namespace rckid