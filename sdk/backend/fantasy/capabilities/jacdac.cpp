#include <rckid/capabilities/jacdac.h>

namespace rckid {

    Jacdac * Jacdac::instance() {
        // JACDAC is not supported in fantasy build
        return nullptr;
    }

    void Jacdac::sendFrame(uint8_t const * data, uint32_t numBytes) {
        // nothing to do
        ASSERT(numBytes <= sizeof(Frame));
        Frame const * f = reinterpret_cast<Frame const *>(data);
        if (! f->checkCrc())
            LOG(LL_ERROR, "Jacdac frame with invalid CRC received");
    }

    void Jacdac::onTick() {
        // nothing to do
    }

    void Jacdac::doEnable() {

    }

    void Jacdac::doDisable() {
        
    }

} // namespace rckid