#include <rckid/capabilities/jacdac.h>

namespace rckid {

    Jacdac * Jacdac::instance() {
        // JACDAC is not supported in fantasy build
        return nullptr;
    }

    void Jacdac::onTick() {
        // nothing to do
    }

    void Jacdac::doEnable() {

    }

    void Jacdac::doDisable() {
        
    }

} // namespace rckid