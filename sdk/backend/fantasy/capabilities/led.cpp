#include <platform.h>

#include <kiddo/capabilities/led.h>

namespace kiddo {

    namespace {
        LED led_;
    }

    LED * LED::instance() {
        return & led_;
    }

    void LED::setActive(bool value) {
        active_ = value;
    }

    void LED::setBrightness(uint8_t value) {
        brightness_ = value;
    }

} // namespace kiddo

