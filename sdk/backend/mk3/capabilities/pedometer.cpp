#include <kiddo/capabilities/pedometer.h>

namespace kiddo {

    Pedometer pedometer_;

    Pedometer * Pedometer::instance() {
        return & pedometer_;
    }

    void Pedometer::reset() {
        count_ = 0;
        UNIMPLEMENTED;
    }
}