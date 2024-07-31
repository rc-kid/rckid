#include <cstring>
#include <thread>

#include <curl/curl.h>

#include "platform.h"
#include "platform/utils/channel.h"

#include <rckid/radio/radio.h>
#include <rckid/radio/wifi.h>

size_t CURLWriteCallback(char *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Hacky way to have two connections in a single process, an extra copy of connection and controller in the curl namespace.  
namespace curl {
    using namespace rckid;
    using namespace rckid::radio;
    #include <rckid/radio/connection.inc.h>
    #include <rckid/radio/controller.inc.h>

    /** A very simple CURL HTTP/HTTPS based client for debugging purposes. 
     
        Should behave the same as the ESP8266 client and test connections & wifi handling in RCKid happs.
     */
    class HTTPClient : public Controller {
    public:
        HTTPClient() {
            // initialize the libcurl
            curl_global_init(CURL_GLOBAL_DEFAULT);        
        }

    protected:

        // accept everything
        void onConnectionRequest(msg::ConnectionOpen const & request) override {
            acceptConnection(request);
            LOG("Accepted connection id: " << (uint32_t)request.requestId << ", param: " << (uint32_t)request.param);
        }

        void onConnectionDataReady(Connection & conn) override {
            LOG("curl connection data received");
            switch (conn.param()) {
                case wifi::CMD_HTTP_GET:
                case wifi::CMD_HTTPS_GET: {
                    if (curl_ == nullptr) {
                        if (conn.canRead<std::string>()) {
                            std::string url = conn.reader().deserialize<std::string>();
                            curl_ = curl_easy_init();
                            curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
                            LOG("Connecting to url " << url);
                            curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, CURLWriteCallback);
                            curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &curlResponse_);
                            // TODO ditch the attributes
                            CURLcode res = curl_easy_perform(curl_);
                            if (res != CURLE_OK)
                                LOG("CURL fail: " << curl_easy_strerror(res));
                            LOG("CURL response: " << curlResponse_);
                        } else {

                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        void onConnectionClosed(Connection & conn, char const *) override {
            LOG("Connection closed remotely, terminating...");
            if (curl_ != nullptr)
                curl_easy_cleanup(curl_);            
        }

        CURL * curl_ = nullptr;
        std::string curlResponse_;


    }; // CurlClient
} // namespace curl

namespace rckid::radio {

    class MsgBytes {
    public:
        uint8_t bytes[32];

        MsgBytes(uint8_t const * from, size_t length) {
            std::memcpy(bytes, from, length);
        }
    }; 

    DeviceId id_;
    Channel<MsgBytes> tx_;
    Channel<MsgBytes> rx_;
    std::thread::id curlThreadId_;

    DeviceId id() { return id_; }


    void initialize(DeviceId deviceId) {
        id_ = deviceId;

        // start the HTTPS client thread
        std::thread t{[](){
            curlThreadId_ = std::this_thread::get_id();
            curl::HTTPClient httpClient{};
            while (true) {
                auto msg = tx_.read(); 
                LOG("curl rx: " << Writer::hex{msg.bytes, 32});
                if (curl::Controller::instance_ != nullptr) {
                    curl::Controller::instance_->onMessageReceived(msg.bytes);
                    curl::Controller::instance_->loop();
                }
            }
        }};
        t.detach();
    }

    void enable(bool silent) {
        // don't do anything, curl can't be disabled

    }

    void disable() {
        // don't do anything, curl can't be disabled
    }

    void transmit(DeviceId target, uint8_t const * msg, size_t length) {
        // figure out which thread we are in and transmit to the corresponding channel
        if (std::this_thread::get_id() == curlThreadId_) {
            LOG("curl tx: (" << length << "): " << Writer::hex{msg, length});
            MsgBytes buffer{msg, length};
            rx_.write(std::move(buffer));
            if (curl::Controller::instance_ != nullptr)
                curl::Controller::instance_->onTransmitSuccess();

        } else {
            LOG("tx: (" << length << "): " << Writer::hex{msg, length});
            MsgBytes buffer{msg, length};
            tx_.write(std::move(buffer));
            if (Controller::instance_ != nullptr)
                Controller::instance_->onTransmitSuccess();
        }
    }

    void loop() {
        while (rx_.canRead()) {
            MsgBytes msg = rx_.read();
            LOG("rx: " << Writer::hex{msg.bytes, 32});
            if (Controller::instance_ != nullptr)
                Controller::instance_->onMessageReceived(msg.bytes);
        }
        if (Controller::instance_ != nullptr)
            Controller::instance_->loop();
    }

} // rckid::radio
