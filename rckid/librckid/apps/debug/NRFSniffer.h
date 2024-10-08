#pragma once

#include "rckid/app.h"


#include <common/radio/radio.h>
#include <common/radio/nrf.h>

namespace rckid {

    /** A simple app that verifies communication with the NRF radio and allows some basic wireless monitoring and testing. 
     */
    class NRFSniffer : public App<FrameBuffer<ColorRGB>>, public radio::Controller {
    public:
        static NRFSniffer * create() { return new NRFSniffer{}; }
    protected:

        void onFocus() override {
            App::onFocus();
            //spi::initialize(GPIO16, GPIO19, GPIO18);
            // TODO check if we need this
            //spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
            /*
            if (!radio_.initializeESB("  RK1", "  RK1", 87)) {
                x_ = radio_.getStatus().raw;
            } else {
                x_ = 255;
            }
            radio_.setAddressLength(3);
            radio_.standby();
            gpio::setAsInputPullup(GPIO17);
            */
            x_ = radio::nrf().getStatus().raw;

        }

        void onBlur() override {
            App::onBlur();
            //radio_.powerDown();
        } 

        void update() override {
            App::update();
            //radio::loop();
            if (pressed(Btn::A)) {
                //uint32_t x = save_and_disable_interrupts();
                /*
                x_ = radio::nrf().getStatus().raw;
                char addr[] = {0,0,0,0,0,0};
                radio::nrf().txAddress(addr);
                LOG("  tx addr:" << addr);    
                radio::nrf().rxAddress(addr);
                LOG("  rx addr:" << addr);    
                LOG("  channel: " << (uint32_t)radio::nrf().channel());
                */
                radio::msg::Ping msg{0, msgId_++, 0x11223344};
//                radio::msg::ConnectionSend msg{56, 12};
                radio::sendMessage('1', msg);
                /*
                uint8_t buf[32];
                for (int i = 0; i < 32; ++i)
                    buf[i] = msgId_;
                ++msgId_;
                radio_.transmitNoAck(buf, 32);
                radio_.enableTransmitter();
                */

               //restore_interrupts(x);

            } 
            if (pressed(Btn::Up)) {
                radio::status_ = radio::nrf().getStatus().raw;
            }
            if (pressed(Btn::Down)) {
                radio::nrf().flushTx();
                radio::status_ = radio::nrf().getStatus().raw;
            }
            /*
            if (gpio::read(GPIO17) == 0) {
                ++irqs_; 
                x_ = radio_.clearIrq();
                radio_.flushTx();
            }
            */
        }

        /* = 2e = state after transmitNoAck (which is received)
             1e in the middle, have multiple IRQs, why? 1e forever when not received?

             the forever is likely because we reset the irqs, which means that the packet still stays in the queue and since we have enabled the transmitter, it will get retransmitted. 

             there is actually no retransmit in the ESB settings now
        
            
        
         */

        void draw() override {
            driver_.fill();
            driver_.textMultiline(0,0) << "Status:    " << Writer::hex{radio::status_} << "\n" << 
                                          "MSG ID:    " << msgId_ << "\n" << 
                                          "Errors:    " << errors_ << "\n" <<
                                          "Transmits: " << transmits_ << "\n";
        }


        void onTransmitFail() override {
            ++errors_;
        }

        void onTransmitSuccess() override {
            ++transmits_;
        }


        uint8_t x_;
        uint8_t msgId_ = 0;
        uint32_t errors_ = 0;
        uint32_t transmits_ = 0;

    }; 

}