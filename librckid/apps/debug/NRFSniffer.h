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
            spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        }

        void onBlur() override {
            App::onBlur();
            //radio_.powerDown();
        } 

        void update() override {
            if (pressed(Btn::A)) {
                //char buffer[10];
                //radio_.txAddress(buffer);
                if (!radio_.initialize("rckid", "rckid"))
                    x_ = radio_.getStatus().raw;
                else
                    x_ = 512;
            }
        }

        void draw() override {
            driver_.fill();
            driver_.textMultiline(0,0) << "Status: " << x_;
        }

        platform::NRF24L01 radio_{GPIO21, GPIO20};

        unsigned x_;
    }; 


}