#include <pico/cyw43_arch.h>

#include <rckid/wifi.h>

namespace rckid {

    /** The wifi driver uses different authentication values in the wifi scan results (not very documented) and different values for the wifi connect.
     */
    static WiFi::AuthMode scanAuthModeToConnectAuthMode(uint8_t authMode) {
        switch (authMode) {
            case 0: return WiFi::AuthMode::Open;
            case 3: return WiFi::AuthMode::WPA_TKIP_PSK;
            case 5: return WiFi::AuthMode::WPA2_AES_PSK;
            case 7: return WiFi::AuthMode::WPA2_MIXED_PSK;
            default:
                return WiFi::AuthMode::Open;
        }
    }

    static int cyw43_wifi_scan_callback(void * env, cyw43_ev_scan_result_t const * result) {
        WiFi::ScanCallback * cb = static_cast<WiFi::ScanCallback *>(env);
        if (result != nullptr) {
            String ssid{reinterpret_cast<char const *>(result->ssid), result->ssid_len};
            if (! ssid.empty()) { // do not support hidden networks
                int16_t rssi = result->rssi;
                WiFi::AuthMode authMode = scanAuthModeToConnectAuthMode(result->auth_mode);
                (*cb)(std::move(ssid), rssi, authMode);
            }
        }
        return 0;
    }

    // WiFi 

    WiFi::Status WiFi::status() const {
        // NOTE we can get the same for AP mode which currently rckid does not support
        int32_t status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        switch (status) {
            case CYW43_LINK_DOWN:
                return Status::Disconnected;
            case CYW43_LINK_JOIN:
            case CYW43_LINK_NOIP:
                return Status::Connecting;
            case CYW43_LINK_UP:
                return Status::Connected;
            default:
                return Status::Off;
        }
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
        cyw43_wifi_scan_options_t options = {};
        int32_t res = cyw43_wifi_scan(&cyw43_state, & options, & scanCallback_, cyw43_wifi_scan_callback);
        return res == 0;
    }

    bool WiFi::connect(String const & ssid, String const & password, AuthMode authMode) {
        LOG(LL_INFO, "WiFi connect to SSID: " << ssid << " password " << password << " auth " << static_cast<uint32_t>(authMode));
        int32_t res = cyw43_arch_wifi_connect_async(ssid.c_str(), password.empty() ? nullptr : password.c_str(), static_cast<uint32_t>(authMode));
        return res == 0;
    }

    uint32_t ipAddress() {
        struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];
        if (netif)
            return netif_ip4_addr(netif)->addr;
        else
            return 0;
    }

    WiFi::~WiFi() {
        enable(false);
        // deinitialie the wifi
        cyw43_arch_deinit();
    }

    void WiFi::tick() {
        cyw43_arch_poll();
        if (scanCallback_ && (cyw43_wifi_scan_active(&cyw43_state) == 0)) {
            LOG(LL_INFO, "WiFi scan complete");
            scanCallback_(String{}, 0, AuthMode::Open);
            scanCallback_ = nullptr;
        };
    }

    WiFi * WiFi::initialize() {
        if (cyw43_arch_init_with_country(CYW43_COUNTRY_WORLDWIDE) != 0) {
            LOG(LL_ERROR, "Failed to initialize WiFi");
            return nullptr;
        }
        return new WiFi{};
    }

} // namespace rckid
