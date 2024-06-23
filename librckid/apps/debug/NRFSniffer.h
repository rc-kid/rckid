#pragma once

#include "rckid/app.h"

#include "nrf24l01.h"

namespace rckid {

    /** A simple app that verifies communication with the NRF radio and allows some basic wireless monitoring and testing. 
     */
    class NRFSniffer : public App<FrameBuffer<ColorRGB>> {
    public:
        static NRFSniffer * create() { return new NRFSniffer{}; }
    protected:

        void onFocus() override {
            App::onFocus();
            spi::initialize(GPIO16, GPIO19, GPIO18);
            // TODO check if we need this
            //spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
            if (!radio_.initialize("RCKID", "RCKID", 56)) {
                x_ = radio_.getStatus().raw;
                radio_.standby();
            } else {
                x_ = 512;
            }
        }

        void onBlur() override {
            App::onBlur();
            //radio_.powerDown();
        } 

        void update() override {
            if (pressed(Btn::A)) {
                uint8_t buf[32];
                for (int i = 0; i < 32; ++i)
                    buf[i] = msgId_;
                ++msgId_;
                radio_.transmit(buf, 32);
                radio_.enableTransmitter();
            }
        }

        void draw() override {
            driver_.fill();
            driver_.textMultiline(0,0) << "Status: " << x_ << "\n" << 
                                          "MSG ID: " << msgId_;
        }

        platform::NRF24L01 radio_{GPIO21, GPIO20};

        unsigned x_;
        uint8_t msgId_ = 0;
    }; 


}