#include <rckid/wifi.h>

namespace rckid {

    WiFi::Status WiFi::status() const {
        return Status::Connected;
    }

    void WiFi::enable([[maybe_unused]] bool value) {
    }

    bool WiFi::scan(WiFi::ScanCallback callback) {
        scanCallback_ = callback;
        return false;
    }

    bool WiFi::connect([[maybe_unused]] String const & ssid, [[maybe_unused]] String const & password, [[maybe_unused]] AuthMode authMode) {
        return true;
    }

    uint32_t WiFi::ipAddress() { 
        return 0; 
    }


    bool WiFi::http_get(char const * hostname, char const * path, RequestCallback callback) {
        return false;
    }

    bool WiFi::https_get(char const * hostname, char const * path, RequestCallback callback) {
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