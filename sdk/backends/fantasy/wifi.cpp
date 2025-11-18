#include <rckid/wifi.h>

namespace rckid {
    bool WiFi::enabled() const {
        return false;
    }

    bool WiFi::connected() const {
        return false;
    }

    void WiFi::enable([[maybe_unused]] bool value) {
    }

    bool WiFi::scan(WiFi::ScanCallback callback) {
        scanCallback_ = callback;
        return false;
    }

    WiFi * WiFi::initialize() {
        return nullptr;
    }

    WiFi::~WiFi() {
    }

    void WiFi::tick() {
    }


}