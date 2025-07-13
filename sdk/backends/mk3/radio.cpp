#include <rckid/radio.h>
#include "i2c.h"

namespace rckid {

    void Radio::enable(bool value) {
        if (value) {
            if (enabled())
                return;
            sendCommand({
                CMD_POWER_UP, 
                CMD_POWER_UP_GPO2OEN | CMD_POWER_UP_XOSCEN, 
                CMD_POWER_UP_FM_RECEIVER
            }, 150);
            busy_ |= RADIO_ENABLED;
            getResponse();
        } else {
            sendCommand({CMD_POWER_DOWN});
            busy_ &= ~RADIO_ENABLED;
            // no need to wait for response, as the radio should be off now
        }
    }

    void Radio::initialize() {
        reset();
        // TODO or should we wait less? 
        cpu::delayMs(100);
        if (::i2c::isPresent(RCKID_FM_RADIO_I2C_ADDRESS)) {
            instance_ = new Radio{};
            LOG(LL_INFO, "  Si4705: " << hex<uint8_t>(RCKID_FM_RADIO_I2C_ADDRESS));
        } else {
            LOG(LL_INFO, "  Si4705: not found");
            instance_ = nullptr;
        }
        gpio::setAsInputPullUp(RP_PIN_RADIO_INT);
        gpio_set_irq_enabled(RP_PIN_RADIO_INT, GPIO_IRQ_EDGE_FALL, true);
    }

    void Radio::reset() {
        gpio::outputLow(RP_PIN_RADIO_RESET);
        cpu::delayMs(10);
        gpio::setAsInputPullUp(RP_PIN_RADIO_RESET);
    }

    void Radio::sendCommand(uint8_t const * cmd, uint8_t cmdSize, uint32_t ctsTime) {
        LOG(LL_INFO, "radio cmd " << hex(cmd[0]));
        i2c::enqueueAndWait(RCKID_FM_RADIO_I2C_ADDRESS, cmd, cmdSize, nullptr, 0);
        cpu::delayMs(ctsTime);
    }

    void Radio::getResponse(uint8_t responseBytes) {
        LOG(LL_INFO, "request response " << responseBytes);
        MaxResponse r;
        i2c::enqueueAndWait(RCKID_FM_RADIO_I2C_ADDRESS, nullptr, 0, (uint8_t *)(& r), responseBytes);
        instance_->status_ = r;
        LOG(LL_INFO, "radio response: " << hex(r.rawResponse()));
    }

    void Radio::irqHandler() {
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