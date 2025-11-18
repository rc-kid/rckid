#include <pico/cyw43_arch.h>

#include <rckid/wifi.h>

namespace rckid {

    static int cyw43_wifi_scan_callback(void * env, cyw43_ev_scan_result_t const * result) {
        WiFi::ScanCallback * cb = static_cast<WiFi::ScanCallback *>(env);
        if (result == nullptr) {
            (*cb)(String{}, 0, WiFi::AuthMode::Open); // signal end of scan
            return 0;
        }
        String ssid{reinterpret_cast<char const *>(result->ssid), result->ssid_len};
        int16_t rssi = result->rssi;
        WiFi::AuthMode authMode = static_cast<WiFi::AuthMode>(result->auth_mode);
        return (*cb)(std::move(ssid), rssi, authMode) ? 0 : 1;
    }

    // WiFi 

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

    void WiFi::enable(bool value) {
        if (value) {
            LOG(LL_INFO, "Enabling WiFi");
            cyw43_arch_enable_sta_mode();
        } else {
            LOG(LL_INFO, "Disabling WiFi");
            cyw43_arch_disable_sta_mode();
        }
    }

    bool WiFi::scan(WiFi::ScanCallback callback) {
        scanCallback_ = callback;
        cyw43_wifi_scan_options_t options = {0};
        int32_t res = cyw43_wifi_scan(&cyw43_state, & options, & scanCallback_, cyw43_wifi_scan_callback);
        return res == 0;
    }

    void WiFi::connect(String const & ssid, String const & password) {
        LOG(LL_INFO, "WiFi connect to SSID: " << ssid);
        //cyw43_arch_wifi_connect_async(ssid.c_str(), password.empty() ? nullptr : password.c_str(), CYW43_WPA_AUTH_DEFAULT);
    }

    WiFi::~WiFi() {
        enable(false);
        // deinitialie the wifi
        cyw43_arch_deinit();
    }

    void WiFi::tick() {
        cyw43_arch_poll();
    }

    WiFi * WiFi::initialize() {
        if (cyw43_arch_init_with_country(CYW43_COUNTRY_WORLDWIDE) != 0) {
            LOG(LL_ERROR, "Failed to initialize WiFi");
            return nullptr;
        }
        return new WiFi{};
    }

} // namespace rckid
