#include <platform/peripherals/nrf24l01.h>

#include "radio.h"
#include "controller.h"

namespace rckid::radio {

    platform::NRF24L01 radio_{RADIO_NRF_PIN_CS, RADIO_NRF_PIN_RXTX};
    DeviceId id_ = BroadcastId;
    DeviceId txAddr_ = BroadcastId;
    
    DeviceId id() { return id_; }

    void initialize(DeviceId deviceId) {
        gpio::setAsInput(RADIO_NRF_PIN_IRQ);
        // initialize radio with own rx and broadcast tx address
        txAddr_ = BroadcastId;
        char rxAddr[5] = { ' ', ' ', 'R', 'K', deviceId};
        char txAddr[5] = { ' ', ' ', 'R', 'K', txAddr_}; 
        radio_.initializeESB(rxAddr, txAddr);
        // since we only support single byte device id, use the smallest tx/rx address format for slightly smaller packages
        radio_.setAddressLength(3); 
        // enable second pie for receiving broadcast messages
        radio_.enablePipe2(BroadcastId, /* esb */ true);
        // set radio into standby mode
        radio_.standby();
    }

    void enable(bool silent) {
        radio_.enableReceiver();
    }

    void disable() {
        radio_.standby();
    }

    void transmit(DeviceId target, uint8_t const * msg, size_t length) {
        if (txAddr_ != target) {
            txAddr_ = target;
            char txAddr[5] = { ' ', ' ', 'R', 'K', txAddr_}; 
            // TODO only do this when not in tx mode  
            radio_.setTxAddress(txAddr);
        }
        // copy the message
        if (msg::requiresAck(static_cast<msg::Id>(msg[0])))
            radio_.transmit(msg, length);
        else 
            radio_.transmitNoAck(msg, length);
        // enter the transmit mode
    } 

    /** IRQ handler for the NRF24L01P
     
        This method can be called either from the polling radio::loop(), or if interrupts are enabled, from the interrupt routine itself. 
     */
    void irqHandler() {
        LOG("NRF IRQ");

        // TODO we should do much much more here, but this is a fair basic test
        radio_.clearDataReadyIrq();
        uint8_t msg[32];
        while (radio_.receive(msg, 32)) {
            LOG("    message: " << Writer::hex{msg, 32});
            if (Controller::singleton_)
                Controller::singleton_->messageReceived(msg);
        }
    }

    void loop() {
        if (gpio::read(RADIO_NRF_PIN_IRQ) == 0)
            irqHandler();
    }

}
