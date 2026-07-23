#include <rckid/capabilities/jacdac.h>

#include <sws_rx.pio.h>
#include <sws_tx.pio.h>

namespace rckid {

    class JACDACImpl : public JACDAC {

    }; // JACDACImpl

    JACDACImpl instance_;

    JACDAC * JACDAC::instance() {
        return &instance_;
    }

    void JACDAC::onTick() {
        UNREACHABLE; // this should never be called, all the functionality is in the JACDACImpl class
    }

} // namespace rckid