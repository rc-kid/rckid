#pragma once

#include "rckid/app.h"


#include <rckid/radio/radio.h>
#include <rckid/radio/wifi.h>


namespace rckid {

    /** A simple app that verifies communication with the NRF radio and allows some basic wireless monitoring and testing. 
     */
    class WiFiTest : public App<FrameBuffer<ColorRGB>>, public radio::wifi::WiFiController {
    public:
        static WiFiTest * create() { return new WiFiTest{}; }

    protected:

        void update() override {
            App::update();
            if (pressed(Btn::A)) {
                /*
                radio::msg::Ping msg{0, msgId_++, 0x11223344};
//                radio::msg::ConnectionSend msg{56, 12};
                radio::sendMessage('1', msg);
                */
               if (conn_ == nullptr) {
                  conn_ = HTTP_GET("https://rc-kid.github.io/", "");
               }
            }
        }

        void draw() override {
            driver_.fill();
            driver_.textMultiline(0,20) << "Conns:     " << numConnections() << "\n";
        }

        void onConnectionAccepted(radio::Connection & conn) override {
            // now the that the connection has been accepted, we can write to it - request the data
            //uint8_t buffer[] = { 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
            //conn.write(buffer, sizeof(buffer));
        }

        void onTransmitFail() override {
//            ++errors_;
        }

        void onTransmitSuccess() override {
//            ++transmits_;
        }

        void onConnectionDataReady(radio::Connection & conn) {
            LOG("Connection data received, terminating...");
            //closeConnection(conn);
        }

        void onConnectionClosed(radio::Connection & conn, char const * extra) override {
            ASSERT(conn_ == & conn);
            conn_ = nullptr;
        }


        uint8_t msgId_ = 0;
/*
        uint8_t x_;
        uint32_t errors_ = 0;
        uint32_t transmits_ = 0;
*/

        radio::Connection * conn_ = nullptr;

       
    }; // rckid::WiFiTest
} // namespace rckid