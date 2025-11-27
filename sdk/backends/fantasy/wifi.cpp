#include <curl/curl.h>

#include <cstdint>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <rckid/wifi.h>

namespace {

    size_t curlWriteCallback(char * ptr, size_t size, size_t nmemb, void * userdata) {
        auto * buffer = static_cast<std::vector<uint8_t> *>(userdata);
        size_t const total = size * nmemb;
        buffer->insert(buffer->end(), reinterpret_cast<uint8_t *>(ptr), reinterpret_cast<uint8_t *>(ptr) + total);
        return total;
    }

} // namespace

namespace rckid {

    class WiFi::ConnectionInternals {
    public:
        ConnectionInternals(char const * hostname, char const * path, bool https, ConnectionCallback callback):
            hostname_{hostname != nullptr ? hostname : ""},
            path_{path != nullptr ? path : ""},
            https_{https},
            callback_{std::move(callback)} {
        }

        void start() {
            std::thread([this]() {
                run();
            }).detach();
        }

    private:
        std::vector<uint8_t> response_;
        String hostname_;
        String path_;
        bool https_;
        ConnectionCallback callback_;

        void run() {
            CURL * curl = curl_easy_init();
            if (!curl) {
                finalize(static_cast<uint32_t>(CURLE_FAILED_INIT) << 16, 0, nullptr);
                delete this;
                return;
            }

            std::string url = https_ ? "https://" : "http://";
            url += hostname_.c_str();
            if (!path_.empty() && path_[0] != '/')
                url.push_back('/');
            url += path_.c_str();

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "rckid-fantasy/1.0");

            CURLcode const result = curl_easy_perform(curl);
            long httpCode = 0;
            if (result == CURLE_OK)
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            curl_easy_cleanup(curl);

            uint32_t const status = (static_cast<uint32_t>(httpCode) & 0xFFFFu) | (static_cast<uint32_t>(result) << 16);
            if (result == CURLE_OK)
                finalize(status, static_cast<uint32_t>(response_.size()), response_.empty() ? nullptr : response_.data());
            else
                finalize(status, 0, nullptr);
            delete this;
        }

        void finalize(uint32_t status, uint32_t size, uint8_t const * data) {
            if (callback_)
                callback_(status, size, data);
        }
    };

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


    WiFi::Connection WiFi::http_get(char const * hostname, char const * path, ConnectionCallback callback) {
        auto * connection = new ConnectionInternals{hostname, path, false, std::move(callback)};
        Connection result{connection};
        connection->start();
        return result;
    }

    WiFi::Connection WiFi::https_get(char const * hostname, char const * path, ConnectionCallback callback) {
        auto * connection = new ConnectionInternals{hostname, path, true, std::move(callback)};
        Connection result{connection};
        connection->start();
        return result;
    }

    WiFi * WiFi::initialize() {
        return new WiFi{};
    }

    WiFi::~WiFi() {
    }

    void WiFi::tick() {
    }


}