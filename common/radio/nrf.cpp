#include <platform/peripherals/nrf24l01.h>

#include "radio.h"

namespace rckid::radio {

    platform::NRF24L01 radio_{RADIO_NRF_PIN_CS, RADIO_NRF_PIN_RXTX};
    Controller * singleton_ = nullptr;

    
    void Controller::initialize(DeviceId deviceId) {
        // initialize radio with own rx and broadcast tx address
        char rxAddr[5] = { ' ', ' ', 'R', 'K', deviceId};
        char txAddr[5] = { ' ', ' ', 'R', 'K', BroadcastId}; 
        radio_.initializeESB(rxAddr, txAddr);
        // since we only support single byte device id, use the smallest tx/rx address format for slightly smaller packages
        radio_.setAddressLength(3); 
        // enable second pie for receiving broadcast messages
        radio_.enablePipe2(BroadcastId, /* esb */ true);
        // set radio into standby mode
        radio_.standby();
    }

    void Controller::enable(bool silent) {
        radio_.enableReceiver();
    }

    void Controller::disable() {
        radio_.standby();
    }

    void Controller::sendMessageRaw(uint8_t const * msg, size_t length) {
        if (msg::requiresAck(static_cast<msg::Id>(msg[0])))
            radio_.transmit(msg, length);
        else 
            radio_.transmitNoAck(msg, length);
    } 

}
