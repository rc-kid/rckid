#include <platform.h>

#include <rckid/capabilities/flashlight.h>

namespace rckid {

    namespace {
        Flashlight flashlight_;
    }

    Flashlight * Flashlight::instance() {
        return & flashlight_;
    }

    void Flashlight::setActive(bool value) {
        active_ = value;
    }

    void Flashlight::setBrightness(uint8_t value) {
        brightness_ = value;
    }

} // namespace rckid

