#include <rckid/wifi.h>

namespace rckid {
    void WiFi::enable() {
    }

    void WiFi::disable() {
    }

    bool WiFi::enabled() const {
        return false;
    }

    bool WiFi::connected() const {
        return false;
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


}