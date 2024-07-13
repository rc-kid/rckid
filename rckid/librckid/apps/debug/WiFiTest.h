#pragma once

#include "rckid/app.h"


#include <common/radio/radio.h>

namespace rckid {

    /** A simple app that verifies communication with the NRF radio and allows some basic wireless monitoring and testing. 
     */
    class WiFiTest : public App<FrameBuffer<ColorRGB>>, public radio::Controller {
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
                openConnection('1');
            }
        }

        void draw() override {
            driver_.fill();
            driver_.textMultiline(0,20) << "Conns:     " << numConnections() << "\n";
        }

        void onConnectionAccepted(radio::Connection & conn) override {
            // now the that the connection has been accepted, we can write to it - request the data
            uint8_t buffer[] = { 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
            conn.write(buffer, sizeof(buffer));
        }

        void onTransmitFail() override {
//            ++errors_;
        }

        void onTransmitSuccess() override {
//            ++transmits_;
        }

        void onConnectionDataReady(radio::Connection & conn) {
            LOG("Connection data received, terminating...");
            terminateConnection(conn);
        }


        uint8_t msgId_ = 0;
/*
        uint8_t x_;
        uint32_t errors_ = 0;
        uint32_t transmits_ = 0;
*/

       
    }; // rckid::WiFiTest
} // namespace rckid