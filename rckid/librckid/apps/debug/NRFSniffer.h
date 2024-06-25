#pragma once

#include <platform/peripherals/nrf24l01.h>

#include "rckid/app.h"


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
            if (!radio_.initializeESB("RCKID", "BSKID", 56)) {
                x_ = radio_.getStatus().raw;
            } else {
                x_ = 255;
            }
            radio_.standby();
            gpio::setAsInputPullup(GPIO17);
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
            if (gpio::read(GPIO17) == 0) {
                ++irqs_; 
                x_ = radio_.clearIrq();
            }
        }

        /* = 2e = state after transmitNoAck (which is received)
             1e in the middle, have multiple IRQs, why? 1e forever when not received?

             the forever is likely because we reset the irqs, which means that the packet still stays in the queue and since we have enabled the transmitter, it will get retransmitted. 

             there is actually no retransmit in the ESB settings now
        
            
        
         */

        void draw() override {
            driver_.fill();
            driver_.textMultiline(0,0) << "Status: " << Writer::hex{x_} << "\n" << 
                                          "MSG ID: " << msgId_ << "\n" << 
                                          "IRQs:   " << irqs_ << "\n";
        }

        platform::NRF24L01 radio_{GPIO21, GPIO20};

        uint8_t x_;
        uint8_t msgId_ = 0;
        unsigned irqs_ = 0;
    }; 


}