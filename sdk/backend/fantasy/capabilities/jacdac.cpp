#include <rckid/capabilities/jacdac.h>

namespace rckid {

    JACDAC * JACDAC::instance() {
        // JACDAC is not supported in fantasy build
        return nullptr;
    }

    void JACDAC::onTick() {
        // nothing to do
    }

} // namespace rckid