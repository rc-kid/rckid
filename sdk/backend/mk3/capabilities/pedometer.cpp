#include <rckid/capabilities/pedometer.h>

namespace rckid {

    Pedometer pedometer_;

    Pedometer * Pedometer::instance() {
        return & pedometer_;
    }

    void Pedometer::reset() {
        count_ = 0;
        UNIMPLEMENTED;
    }
}