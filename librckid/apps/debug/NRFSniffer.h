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
            if (!radio_.initialize("rckid", "rckid"))
                FATAL_ERROR(67);
        }

        void onBlur() override {
            App::onBlur();
            //radio_.powerDown();
        } 

        void draw() override {
            driver_.fill();
        }

        platform::NRF24L01 radio_{GPIO21, GPIO20};
    }; 


}