#include <rckid/radio.h>
#include "i2c.h"

namespace rckid {

    void Radio::enable(bool value) {
        if (value) {
            if (enabled())
                return;
            uint8_t cmd[] = {
                CMD_POWER_UP, 
                CMD_POWER_UP_CTSIEN | CMD_POWER_UP_GPO2OEN | CMD_POWER_UP_XOSCEN, 
                CMD_POWER_UP_FM_RECEIVER
            };
            status_.clearCts();
            expectedResponse_ = ExpectedResponse::Status;
            i2c::enqueue(new i2c::Packet(
                RCKID_FM_RADIO_I2C_ADDRESS,
                sizeof(cmd),
                cmd
            ));
        } else {
            waitNotBusy();
            uint8_t cmd[] = {
                CMD_POWER_UP, 
                0, 
                0
            };
            expectedResponse_ = ExpectedResponse::PowerOff;
            i2c::enqueue(new i2c::Packet(
                RCKID_FM_RADIO_I2C_ADDRESS,
                sizeof(cmd),
                cmd
            ));
        }
    }

    void Radio::setFrequency(uint16_t freq_10kHz) {
        // TODO wait for not busy
        uint8_t cmd[] = {
            CMD_FM_TUNE_FREQ,
            0, // no freeze or fast mode
            platform::highByte(freq_10kHz),
            platform::lowByte(freq_10kHz),
            0, // automatic antenna tuning capacitor value
        };
        status_.clearCts();
        status_.clearStc();
        expectedResponse_ = ExpectedResponse::Status;
        i2c::enqueue(new i2c::Packet(
            RCKID_FM_RADIO_I2C_ADDRESS,
            sizeof(cmd),
            cmd
        ));
    }

    void Radio::seekUp() {
        uint8_t cmd[] = {
            CMD_FM_SEEK_START,
            0x08 | 0x04, // seek up, wrap around
        };
        status_.clearCts();
        status_.clearStc();
        expectedResponse_ = ExpectedResponse::Status;
        i2c::enqueue(new i2c::Packet(
            RCKID_FM_RADIO_I2C_ADDRESS,
            sizeof(cmd),
            cmd
        ));
    }

    void Radio::seekDown() {
        uint8_t cmd[] = {
            CMD_FM_SEEK_START,
            0x04, // seek down, wrap around
        };
        status_.clearCts();
        status_.clearStc();
        expectedResponse_ = ExpectedResponse::Status;
        i2c::enqueue(new i2c::Packet(
            RCKID_FM_RADIO_I2C_ADDRESS,
            sizeof(cmd),
            cmd
        ));
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


    void Radio::irqHandler() {
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
    }

    void Radio::processStatusResponse(uint8_t numBytes) {
        ASSERT(numBytes == 1);
        i2c::readResponse((uint8_t *)&instance_->status_.raw_, numBytes);

    }





} // namespace rckid