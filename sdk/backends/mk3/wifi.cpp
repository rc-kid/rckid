#include <pico/cyw43_arch.h>

#include <rckid/wifi.h>

namespace rckid {
    void WiFi::enable() {
        LOG(LL_INFO, "Enabling WiFi");
        cyw43_arch_enable_sta_mode();
    }

    void WiFi::disable() {
        LOG(LL_INFO, "Disabling WiFi");
        cyw43_arch_disable_sta_mode();
    }

    bool WiFi::enabled() const {
        // NOTE we can get the same for AP mode which currently rckid does not support
        int32_t status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
        switch (status) {
            case CYW43_LINK_DOWN:
            case CYW43_LINK_JOIN:
            case CYW43_LINK_NOIP:
            case CYW43_LINK_UP:
                return true;
            default:
                return false;
        }        
    }

    bool WiFi::connected() const {
        int32_t status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
        return status == CYW43_LINK_UP;
    }

    void WiFi::initialize() {
        if (cyw43_arch_init_with_country(CYW43_COUNTRY_WORLDWIDE) != 0) {
            LOG(LL_ERROR, "Failed to initialize WiFi");
            return;
        }
    }

    WiFi::~WiFi() {
        // deinitialie the wifi
        cyw43_arch_deinit();
    }
}