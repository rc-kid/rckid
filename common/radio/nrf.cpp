#include <cstring>

#include <platform/peripherals/nrf24l01.h>

#include "radio.h"
#include "controller.h"


namespace rckid::radio {

    platform::NRF24L01 radio_{RADIO_NRF_PIN_CS, RADIO_NRF_PIN_RXTX};
    DeviceId id_ = BroadcastId;
    DeviceId txAddr_ = BroadcastId;
    
    DeviceId id() { return id_; }

    void initialize(DeviceId deviceId) {
        // on RCKid initialize the cartridge's SPI
#if (defined ARCH_RCKID)
        spi::initialize(RADIO_NRF_PIN_MISO, RADIO_NRF_PIN_MOSI, RADIO_NRF_PIN_SCK);
        // the line below does not seme to be needed anymore
        //spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
#endif
        gpio::setAsInputPullup(RADIO_NRF_PIN_IRQ);
        // initialize radio with own rx and broadcast tx address
        txAddr_ = BroadcastId;
        char rxAddr[5] = { ' ', ' ', 'R', 'K', deviceId};
        char txAddr[5] = { ' ', ' ', 'R', 'K', txAddr_}; 
        //UNREACHABLE;
        radio_.initializeESB(rxAddr, txAddr, 87);
        // since we only support single byte device id, use the smallest tx/rx address format for slightly smaller packages
        radio_.setAddressLength(3); 
        // enable second pie for receiving broadcast messages
        radio_.enablePipe2(BroadcastId, /* esb */ true);
        // set radio into standby mode
        radio_.standby();
        LOG("Radio initialized");
    }

    void enable(bool silent) {
        LOG("Enabling receiver...");
        radio_.enableReceiver();
    }

    void disable() {
        LOG("Disabling radio");
        radio_.standby();
    }

    void transmit(DeviceId target, uint8_t const * msg, size_t length) {
        LOG("transmit: " << Writer::hex{msg, length});
        if (txAddr_ != target) {
            LOG("Switching address to " << (uint32_t)target);
            txAddr_ = target;
            char txAddr[5] = { ' ', ' ', 'R', 'K', txAddr_}; 
            // TODO only do this when not in tx mode  
            radio_.setTxAddress(txAddr);
        }
        uint8_t xmsg[32];
        if (length < 32) {
            memcpy(xmsg, msg, length);
            msg = xmsg;
            length = 32;
        }
        if (msg::requiresAck(static_cast<msg::Id>(msg[0])))
            radio_.transmit(msg, length);
        else 
            radio_.transmitNoAck(msg, length);
        // enter the transmit mode
        radio_.enableTransmitter();
    } 

    /** IRQ handler for the NRF24L01P

        The method itself can either be registered as an interrupt, or be called from the loop when the IRQ pin is detected low. Upon each IRQ     
     */
    void irqHandler() {
        LOG("NRF IRQ");
        // see what the fuss is about
        platform::NRF24L01::Status status = radio_.clearIrq();
        radio_.flushTx();
        if (Controller::instance_ != nullptr) {
            if (status.txDataFailIrq())
                Controller::instance_->onTransmitFail();
            if (status.txDataSentIrq())
                Controller::instance_->onTransmitSuccess();
        }
        uint8_t msg[32];
        while (radio_.receive(msg, 32)) {
            LOG("    message: " << Writer::hex{msg, 32});
            if (Controller::instance_ != nullptr)
                Controller::instance_->onMessageReceived(msg);
        }
    }

    void loop() {
        if (gpio::read(RADIO_NRF_PIN_IRQ) == 0)
            irqHandler();
    }


    platform::NRF24L01& nrf() {
        return radio_;
    }

}
