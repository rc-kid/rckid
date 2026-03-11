#include <pico/cyw43_arch.h>
#include <lwip/apps/http_client.h>
#include <lwip/altcp_tls.h>

#include <rckid/capabilities/wifi.h>

namespace rckid {

    namespace {

        WiFi * wiFiInstance = nullptr;

        WiFi::ScanCallback scanCallback;

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
    } // anonymous namespace

    class RM2Connection : public WiFi::Connection {
    public:

        RM2Connection(String hostname, String path, Callback cb):
            Connection{std::move(cb)},
            hostname_{std::move(hostname)},
            path_{std::move(path)}
        {
            response_.reserve(1024);
        }

        int32_t status() const override { return status_; }


        ~RM2Connection() {
            delete httpc.altcp_allocator;
        }

        
    private:

        friend class WiFi;

        std::vector<uint8_t> response_;
        String hostname_;
        String path_;
        httpc_connection_t httpc{};
        httpc_state_t * conn = nullptr;
        int32_t status_ = Connection::IN_PROGRESS;


        // Override altcp_tls_alloc to set sni
        static struct ::altcp_pcb * altcp_tls_alloc_sni(void *arg, u8_t ip_type) {
            ASSERT(arg != nullptr);
            RM2Connection * ci = static_cast<RM2Connection *>(arg);
            LOG(LL_WIFI, "Allocating TLS PCB for hostname: " << ci->hostname_);
            ::altcp_pcb * pcb = altcp_tls_alloc(tls_config, ip_type);
            mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(pcb), ci->hostname_.c_str());
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
            RM2Connection * ci = static_cast<RM2Connection *>(arg);
            while (p != nullptr) {
                LOG(LL_WIFI, "Request " << ci << " received " << (p->len) << " bytes, lwip err " << err);
                ci->response_.insert(ci->response_.end(), (uint8_t *)p->payload, (uint8_t *)p->payload + p->len);
                p = p->next;
            }
            return ERR_OK;
        }    

        static void cyw43_wifi_result_fn(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
            ASSERT(arg != nullptr);
            RM2Connection * ci = static_cast<RM2Connection *>(arg);
            srv_res = srv_res | ((uint32_t)(httpc_result) << 16);
            LOG(LL_WIFI, "Request " << ci << " finished: " << srv_res << "(httpc " << (int32_t)httpc_result << ") received " << rx_content_len << " bytes, lwip err " << err);
            ci->status_ = srv_res;
            if (httpc_result == HTTPC_RESULT_OK) {
                ASSERT(rx_content_len == ci->response_.size());
                ci->cb_(ci, ci->response_.size(), ci->response_.data());
            } else {
                ci->cb_(ci, 0, nullptr);
            }
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

    }; // rckid::RM2Connection

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

    WiFi * WiFi::instance() {
        if (wiFiInstance == nullptr) {
            if (cyw43_arch_init_with_country(CYW43_COUNTRY_WORLDWIDE) == 0)
                wiFiInstance = new WiFi{};
            else 
                LOG(LL_ERROR, "Failed to initialize WiFi");
        }
        return wiFiInstance;
    }


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
        scanCallback = callback;
        cyw43_wifi_scan_options_t options = {};
        int32_t res = cyw43_wifi_scan(&cyw43_state, & options, & scanCallback, cyw43_wifi_scan_callback);
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
    unique_ptr<WiFi::Connection> WiFi::http_get(String hostname, String path, Connection::Callback callback) {
        auto result = std::make_unique<RM2Connection>(std::move(hostname), std::move(path), std::move(callback));
        result->httpc.headers_done_fn = RM2Connection::cyw43_wifi_receive_header_fn;
        result->httpc.result_fn = RM2Connection::cyw43_wifi_result_fn;
        int32_t res = httpc_get_file_dns(
            result->hostname_.c_str(),
            80,
            result->path_.c_str(),
            & result->httpc,
            RM2Connection::cyw43_wifi_receive_fn,
            result.get(),
            & result->conn
        );
        if (res == ERR_OK)
            return result;
        else
            return nullptr;
    }

    unique_ptr<WiFi::Connection> WiFi::https_get(String hostname, String path, Connection::Callback callback) {
        auto result = std::make_unique<RM2Connection>(std::move(hostname), std::move(path), std::move(callback));
        result->httpc.headers_done_fn = RM2Connection::cyw43_wifi_receive_header_fn;
        result->httpc.result_fn = RM2Connection::cyw43_wifi_result_fn;
        result->httpc.altcp_allocator = new altcp_allocator_t{};
        result->httpc.altcp_allocator->alloc = RM2Connection::altcp_tls_alloc_sni;
        result->httpc.altcp_allocator->arg = (void *)result.get();
        int32_t res = httpc_get_file_dns(
            result->hostname_.c_str(), 
            443, 
            result->path_.c_str(), 
            & result->httpc,
            RM2Connection::cyw43_wifi_receive_fn,
            result.get(), // callback arg
            & result->conn);
        LOG(LL_INFO, "HTTPS GET result: " << res);
        if (res == ERR_OK)
            return result;
        else
            return nullptr;
    }


    WiFi::~WiFi() {
        enable(false);
        // deinitialie the wifi
        cyw43_arch_deinit();
    }

    void WiFi::onTick() {
        cyw43_arch_poll();
        if (scanCallback && (cyw43_wifi_scan_active(&cyw43_state) == 0)) {
            LOG(LL_WIFI, "WiFi scan complete");
            scanCallback(String{}, 0, AuthMode::Open);
            scanCallback = nullptr;
        };
    }

} // namespace rckid
