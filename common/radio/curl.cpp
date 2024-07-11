#include <cstring>
#include <thread>

#include "platform.h"
#include "platform/utils/channel.h"

#include "radio.h"

// Hacky way to have two connections in a single process, an extra copy of connection and controller in the curl namespace.  
namespace curl {
    using namespace rckid;
    using namespace rckid::radio;
    #include "connection.inc.h"
    #include "controller.inc.h"


    /** A very simple CURL HTTP/HTTPS based client for debugging purposes. 
     
        Should behave the same as the ESP8266 client and test connections & wifi handling in RCKid happs. 
     */
    class HTTPClient : public Controller {
    protected:

        // accept everything
        void onConnectionRequest(msg::ConnectionOpen const & request) override {
            acceptConnection(request);
        }
        

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
                if (curl::Controller::instance_ != nullptr)
                    curl::Controller::instance_->onMessageReceived(msg.bytes);
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
    }

} // rckid::radio
