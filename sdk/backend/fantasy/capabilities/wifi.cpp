#ifdef RCKID_CAPABILITY_WIFI

#include <curl/curl.h>

#include <rckid/capabilities/wifi.h>

namespace {

    size_t curlWriteCallback(char * ptr, size_t size, size_t nmemb, void * userdata) {
        auto * buffer = static_cast<std::vector<uint8_t> *>(userdata);
        size_t const total = size * nmemb;
        buffer->insert(buffer->end(), reinterpret_cast<uint8_t *>(ptr), reinterpret_cast<uint8_t *>(ptr) + total);
        return total;
    }

} // namespace

namespace rckid {

    class CurlConnection : public WiFi::Connection {
    public:

        CurlConnection(String hostname, String path, bool https, Callback cb):
            Connection{std::move(cb)},
            hostname_{std::move(hostname)},
            path_{std::move(path)},
            https_{https},
            curl_{curl_easy_init()} {
        }

        int32_t status() const override { return status_; }

        bool good() const { return curl_ != nullptr; }

    private:

        friend class WiFi;

        void start() {
            std::thread([this]() {

                std::string url = https_ ? "https://" : "http://";
                url += hostname_.c_str();
                if (!path_.empty() && path_[0] != '/')
                    url.push_back('/');
                url += path_.c_str();

                curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, curlWriteCallback);
                curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_);
                curl_easy_setopt(curl_, CURLOPT_USERAGENT, "rckid-fantasy/1.0");

                CURLcode const result = curl_easy_perform(curl_);
                long httpCode = 0;
                if (result == CURLE_OK)
                    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpCode);
                curl_easy_cleanup(curl_);

                uint32_t const status = (static_cast<uint32_t>(httpCode) & 0xFFFFu) | (static_cast<uint32_t>(result) << 16);
                if (result == CURLE_OK)
                    finalize(status, static_cast<uint32_t>(response_.size()), response_.empty() ? nullptr : response_.data());
                else
                    finalize(status, 0, nullptr);
            }).detach();
        }

        void finalize(int32_t status, uint32_t size, uint8_t const * data) {
            status_ = status;
            cb_(this, size, data);
        }

        std::vector<uint8_t> response_;
        String hostname_;
        String path_;
        bool https_;
        int32_t status_ = IN_PROGRESS;
        CURL * curl_ = nullptr;
        
    }; // rckid::CurlConnection

    WiFi * instance_ = nullptr;

    WiFi * WiFi::instance() {
        if (instance_ == nullptr)
            instance_ = new WiFi();
        return instance_;
    }

    WiFi::~WiFi() {
        ASSERT(instance_ == this);
        instance_ = nullptr;
    }

    WiFi::Status WiFi::status() const {
        return Status::Connected;
    }

    void WiFi::enable([[maybe_unused]] bool value) {
    }

    bool WiFi::scan(WiFi::ScanCallback callback) {
        callback("RCKid_WiFi", -50, AuthMode::WPA2_AES_PSK);
        return true;
    }

    bool WiFi::connect([[maybe_unused]] String const & ssid, [[maybe_unused]] String const & password, [[maybe_unused]] AuthMode authMode) {
        return true;
    }

    uint32_t WiFi::ipAddress() { 
        return 0; 
    }

    unique_ptr<WiFi::Connection> WiFi::http_get(String hostname, String path, Connection::Callback callback) {
        auto result = std::make_unique<CurlConnection>(std::move(hostname), std::move(path), false, std::move(callback));
        if (! result->good())
            return nullptr;
        result->start();
        return result;
    }

    unique_ptr<WiFi::Connection> WiFi::https_get(String hostname, String path, Connection::Callback callback) {
        auto result = std::make_unique<CurlConnection>(std::move(hostname), std::move(path), true, std::move(callback));
        if (! result->good())
            return nullptr;
        result->start();
        return result;
    }

    void WiFi::onTick() {
        // no need to do anything as connection callbacks are in separate threads
    }

}

#else

#include <rckid/capabilities/wifi.h>

namespace rckid {

    WiFi * WiFi::instance() { return nullptr; }

    WiFi::~WiFi() = default;

    WiFi::Status WiFi::status() const { return Status::Off; }

    void WiFi::enable(bool) {
    }

    bool WiFi::scan(ScanCallback) {
        return false;
    }

    bool WiFi::connect(String const & ssid, String const & password, AuthMode authMode) {
        return false;
    }

    uint32_t WiFi::ipAddress() { 
        return 0; 
    }

} // namespace rckid

#endif