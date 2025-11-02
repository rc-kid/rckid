#include <rckid/radio.h>
#include "i2c.h"
#include "audio/codec.h"

namespace rckid {

    void Radio::enable(bool value) {
        if (value) {
            Codec::playbackLineInDirect();
            // we need to generate master clock for the codec to work - not sure why
            Codec::enableMasterClock(48000);
            if (enabled())
                return;
            sendCommand({
                CMD_POWER_UP, 
                CMD_POWER_UP_GPO2OEN | CMD_POWER_UP_XOSCEN, 
                CMD_POWER_UP_FM_RECEIVER
            }, 150);
            busy_ |= RADIO_ENABLED;
            getResponse();
            setProperty(PROP_FM_RSQ_INT_SOURCE, FM_RSQ_INT_SOURCE_BLENDIEN | FM_RSQ_INT_SOURCE_SNRHIEN | FM_RSQ_INT_SOURCE_SNRLIEN | FM_RSQ_INT_SOURCE_RSSIHIEN | FM_RSQ_INT_SOURCE_RSSILIEN);
            setProperty(PROP_FM_RDS_INT_SOURCE, FM_RDS_INT_SOURCE_RDSRECV);
            setProperty(PROP_FM_RDS_CONFIG, FM_RDS_RDSEN);
            // somehow the repeated int handling is necessary here so far to receive the interrupts
            // figure out why
            setProperty(PROP_GPO_IEN, GPO_IEN_STCIEN | GPO_IEN_RDSIEN | GPO_IEN_RSQIEN | (1 << 10) | (1 << 11));
            setProperty(PROP_FM_RDS_INT_FIFO_COUNT, 20);

        } else {
            Codec::stop();
            sendCommand({CMD_POWER_DOWN});
            busy_ &= ~RADIO_ENABLED;
            // no need to wait for response, as the radio should be off now
        }
    }

    void Radio::initialize() {
        if (::i2c::isPresent(RCKID_FM_RADIO_I2C_ADDRESS)) {
            instance_ = new Radio{};
        } else {
            LOG(LL_INFO, "FM Radio chip not detected");
            instance_ = nullptr;
        }
        gpio::setAsInputPullUp(RP_PIN_RADIO_INT);
        gpio_set_irq_enabled(RP_PIN_RADIO_INT, GPIO_IRQ_EDGE_FALL, true);
    }

    void Radio::reset() {
        Codec::setGPIO1(true);
        cpu::delayMs(10);
        Codec::setGPIO1(false); // radio reset is active low
        cpu::delayMs(10);
        Codec::setGPIO1(true);
        cpu::delayMs(10);

        /*
        gpio::outputLow(RP_PIN_RADIO_RESET);
        cpu::delayMs(10);
        gpio::setAsInputPullUp(RP_PIN_RADIO_RESET);
        */
    }

    void Radio::sendCommand(uint8_t const * cmd, uint8_t cmdSize, uint32_t ctsTime) {
        i2c::enqueueAndWait(RCKID_FM_RADIO_I2C_ADDRESS, cmd, cmdSize, nullptr, 0);
        cpu::delayMs(ctsTime);
    }

    void Radio::getResponse(uint8_t responseBytes) {
        MaxResponse r;
        i2c::enqueueAndWait(RCKID_FM_RADIO_I2C_ADDRESS, nullptr, 0, (uint8_t *)(& r), responseBytes);
        instance_->status_ = r;
    }

    bool Radio::update() {
        // TODO also read the FM_RSQ_status periodically for stereo
        // and deal with rds 
        if (!irq_)
            return false;
        irq_ = false;
        getStatus();
        if (status_.stcInt())
            getTuneStatus();
        if (status_.rsqInt())
            getRSQStatus();
        if (status_.rdsInt())
            getRDSStatus();
        return true;
    }

    void Radio::irqHandler() {
        irq_ = true;
        /*
        i2c::enqueue(RCKID_FM_RADIO_I2C_ADDRESS, & cmd, sizeof(cmd), 1, [](uint8_t numBytes) {
            i2c::getTransactionResponse(reinterpret_cast<uint8_t*>(& instance_->status_), 1);
            ++irqs_;
            irqs_ = instance_->status_.rawResponse();
        }); */
        /*
        switch (instance_->expectedResponse_) {
            case ExpectedResponse::Status: {
                uint8_t cmd = CMD_GET_INT_STATUS;
                i2c::enqueue(new i2c::Packet(
                    RCKID_FM_RADIO_I2C_ADDRESS,
                    sizeof(cmd),
                    &cmd,
                    1,
                    processStatusResponse
                ));        
                break;
            }
            default:
                UNIMPLEMENTED;
        }
        instance_->expectedResponse_ = ExpectedResponse::None;
        */
    }

} // namespace rckid