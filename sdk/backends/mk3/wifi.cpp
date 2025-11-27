#include <pico/cyw43_arch.h>
#include <lwip/apps/http_client.h>
#include <lwip/altcp_tls.h>

#include <rckid/wifi.h>

namespace rckid {

    namespace {

        static altcp_tls_config * tls_config = nullptr;

        // from pico examples
        // This is the PUBLIC root certificate exported from a browser
        // Note that the newlines are needed
        static constexpr uint8_t rootCertificate[] = "-----BEGIN CERTIFICATE-----\n\
        MIIC+jCCAn+gAwIBAgICEAAwCgYIKoZIzj0EAwIwgbcxCzAJBgNVBAYTAkdCMRAw\n\
        DgYDVQQIDAdFbmdsYW5kMRIwEAYDVQQHDAlDYW1icmlkZ2UxHTAbBgNVBAoMFFJh\n\
        c3BiZXJyeSBQSSBMaW1pdGVkMRwwGgYDVQQLDBNSYXNwYmVycnkgUEkgRUNDIENB\n\
        MR0wGwYDVQQDDBRSYXNwYmVycnkgUEkgUm9vdCBDQTEmMCQGCSqGSIb3DQEJARYX\n\
        c3VwcG9ydEByYXNwYmVycnlwaS5jb20wIBcNMjExMjA5MTEzMjU1WhgPMjA3MTEx\n\
        MjcxMTMyNTVaMIGrMQswCQYDVQQGEwJHQjEQMA4GA1UECAwHRW5nbGFuZDEdMBsG\n\
        A1UECgwUUmFzcGJlcnJ5IFBJIExpbWl0ZWQxHDAaBgNVBAsME1Jhc3BiZXJyeSBQ\n\
        SSBFQ0MgQ0ExJTAjBgNVBAMMHFJhc3BiZXJyeSBQSSBJbnRlcm1lZGlhdGUgQ0Ex\n\
        JjAkBgkqhkiG9w0BCQEWF3N1cHBvcnRAcmFzcGJlcnJ5cGkuY29tMHYwEAYHKoZI\n\
        zj0CAQYFK4EEACIDYgAEcN9K6Cpv+od3w6yKOnec4EbyHCBzF+X2ldjorc0b2Pq0\n\
        N+ZvyFHkhFZSgk2qvemsVEWIoPz+K4JSCpgPstz1fEV6WzgjYKfYI71ghELl5TeC\n\
        byoPY+ee3VZwF1PTy0cco2YwZDAdBgNVHQ4EFgQUJ6YzIqFh4rhQEbmCnEbWmHEo\n\
        XAUwHwYDVR0jBBgwFoAUIIAVCSiDPXut23NK39LGIyAA7NAwEgYDVR0TAQH/BAgw\n\
        BgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwCgYIKoZIzj0EAwIDaQAwZgIxAJYM+wIM\n\
        PC3wSPqJ1byJKA6D+ZyjKR1aORbiDQVEpDNWRKiQ5QapLg8wbcED0MrRKQIxAKUT\n\
        v8TJkb/8jC/oBVTmczKlPMkciN+uiaZSXahgYKyYhvKTatCTZb+geSIhc0w/2w==\n\
        -----END CERTIFICATE-----\n";
    }


    class WiFi::ConnectionInternals {
    public:
        std::vector<uint8_t> response;

        String hostname;

        httpc_connection_t httpc{};
        httpc_state_t * conn = nullptr;

        ConnectionCallback cb;

        ConnectionInternals(char const * hostname, ConnectionCallback callback):
            hostname{hostname}, 
            cb{std::move(callback)}
        {
            response.reserve(1024);
        }

        ~ConnectionInternals() {
            delete httpc.altcp_allocator;
        }

    }; // WiFi::ConnectionInternals

    Writer & operator << (Writer & w, WiFi::ConnectionInternals const & ci) {
        w << hex((uint32_t)(&ci));
        return w;
    }

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

    // Override altcp_tls_alloc to set sni
    static struct altcp_pcb *altcp_tls_alloc_sni(void *arg, u8_t ip_type) {
        ASSERT(arg != nullptr);
        WiFi::ConnectionInternals * ci = static_cast<WiFi::ConnectionInternals *>(arg);
        LOG(LL_WIFI, "Allocating TLS PCB for hostname: " << ci->hostname);
        altcp_pcb * pcb = altcp_tls_alloc(tls_config, ip_type);
        mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(pcb), ci->hostname.c_str());
        return pcb;
    }

    static err_t cyw43_wifi_receive_header_fn(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len) {
        /*
        LOG(LL_WIFI, "headers received ");
        LOG(LL_WIFI, "  hdr_len: " << hdr_len);
        LOG(LL_WIFI, "  content_len: " << content_len);
        while (hdr != nullptr) {
            LOG(LL_WIFI, "  pbuf tot_len: " << hdr->tot_len);
            LOG(LL_WIFI, "  pbuf len: " << hdr->len);
            for (uint32_t i = 0; i < hdr->len; ++i) {
                debugWrite() << ((char *)(hdr->payload))[i];
            }
            hdr = hdr->next;
        }
            */
        /*
        EXAMPLE_HTTP_REQUEST_T *req = (EXAMPLE_HTTP_REQUEST_T*)arg;
        if (req->headers_fn) {
            return req->headers_fn(connection, req->callback_arg, hdr, hdr_len, content_len);
        }*/
        return ERR_OK;
    }

    static err_t cyw43_wifi_receive_fn(void *arg, altcp_pcb * conn, pbuf * p, err_t err) {
        ASSERT(arg != nullptr);
        WiFi::ConnectionInternals * ci = static_cast<WiFi::ConnectionInternals *>(arg);
        while (p != nullptr) {
            LOG(LL_WIFI, "Request " << *ci << " received " << (p->len) << " bytes, lwip err " << err);
            ci->response.insert(ci->response.end(), (uint8_t *)p->payload, (uint8_t *)p->payload + p->len);
            p = p->next;
        }
        return ERR_OK;
    }    

    static void cyw43_wifi_result_fn(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
        ASSERT(arg != nullptr);
        WiFi::ConnectionInternals * ci = static_cast<WiFi::ConnectionInternals *>(arg);
        srv_res = srv_res | ((uint32_t)(httpc_result) << 16);
        LOG(LL_WIFI, "Request " << *ci << " finished: " << srv_res << "(httpc " << (int32_t)httpc_result << ") received " << rx_content_len << " bytes, lwip err " << err);
        if (httpc_result == HTTPC_RESULT_OK) {
            ASSERT(rx_content_len == ci->response.size());
            ci->cb(srv_res, ci->response.size(), ci->response.data());
        } else {
            ci->cb(srv_res, 0, nullptr);
        }
        delete ci;
        /*
        EXAMPLE_HTTP_REQUEST_T *req = (EXAMPLE_HTTP_REQUEST_T*)arg;
        HTTP_DEBUG("result %d len %u server_response %u err %d\n", httpc_result, rx_content_len, srv_res, err);
        req->complete = true;
        req->result = httpc_result;
        if (req->result_fn) {
            req->result_fn(req->callback_arg, httpc_result, rx_content_len, srv_res, err);
        }
            */
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
            tls_config = altcp_tls_create_config_client(nullptr, 0);
        } else {
            LOG(LL_INFO, "Disabling WiFi");
            cyw43_arch_disable_sta_mode();
            altcp_tls_free_config(tls_config);
            tls_config = nullptr;
        }
    }

    bool WiFi::scan(WiFi::ScanCallback callback) {
        scanCallback_ = callback;
        cyw43_wifi_scan_options_t options = {};
        int32_t res = cyw43_wifi_scan(&cyw43_state, & options, & scanCallback_, cyw43_wifi_scan_callback);
        return res == 0;
    }

    bool WiFi::connect(String const & ssid, String const & password, AuthMode authMode) {
        LOG(LL_WIFI, "WiFi connect to SSID: " << ssid << " password " << password << " auth " << static_cast<uint32_t>(authMode));
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

    /* Notes:

       - the http_connection_t must be alive for as long as the connection is
     
     */
    WiFi::Connection WiFi::http_get(char const * hostname, char const * path, ConnectionCallback callback) {
        Connection ci{new ConnectionInternals{hostname, callback}};
        ci.c_->httpc.headers_done_fn = cyw43_wifi_receive_header_fn;
        ci.c_->httpc.result_fn = cyw43_wifi_result_fn;
        int32_t result = httpc_get_file_dns(
            ci.c_->hostname.c_str(),
            80,
            path,
            & ci.c_->httpc,
            cyw43_wifi_receive_fn,
            ci.c_,
            & ci.c_->conn
        );
        if (result == ERR_OK)
            return ci;
        else
            return Connection{nullptr};
        /*

        static httpc_connection_t settings = {};
        settings.headers_done_fn = cyw43_wifi_receive_header_fn;
        settings.result_fn = cyw43_wifi_result_fn;
        httpc_state_t * conn;
        int32_t result = httpc_get_file_dns(
            hostname, 
            80, 
            path, 
            & settings,
            cyw43_wifi_receive_fn,
            nullptr, // callback arg
            & conn);
        LOG(LL_INFO, "HTTP GET result: " << result);
        return result == ERR_OK;
        */
    }

    WiFi::Connection WiFi::https_get(char const * hostname, char const * path, ConnectionCallback callback) {
        Connection ci{new ConnectionInternals{hostname, callback}};
        ci.c_->httpc.headers_done_fn = cyw43_wifi_receive_header_fn;
        ci.c_->httpc.result_fn = cyw43_wifi_result_fn;
        ci.c_->httpc.altcp_allocator = new altcp_allocator_t{};
        ci.c_->httpc.altcp_allocator->alloc = altcp_tls_alloc_sni;
        ci.c_->httpc.altcp_allocator->arg = (void *)ci.c_;
        int32_t result = httpc_get_file_dns(
            ci.c_->hostname.c_str(), 
            443, 
            path, 
            & ci.c_->httpc,
            cyw43_wifi_receive_fn,
            ci.c_, // callback arg
            & ci.c_->conn);
        LOG(LL_INFO, "HTTPS GET result: " << result);
        if (result == ERR_OK)
            return ci;
        else
            return Connection{nullptr};
    }


    WiFi::~WiFi() {
        enable(false);
        // deinitialie the wifi
        cyw43_arch_deinit();
    }

    void WiFi::tick() {
        cyw43_arch_poll();
        if (scanCallback_ && (cyw43_wifi_scan_active(&cyw43_state) == 0)) {
            LOG(LL_WIFI, "WiFi scan complete");
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
