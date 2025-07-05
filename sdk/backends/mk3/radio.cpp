#include <rckid/radio.h>
#include "i2c.h"

namespace rckid {

    void Radio::enable(bool value) {
        if (value && status_ == Status::Off) {
            uint8_t cmd[] = {
                CMD_POWER_UP, 
                CMD_POWER_UP_CTSIEN | CMD_POWER_UP_GPO2OEN | CMD_POWER_UP_XOSCEN, 
                CMD_POWER_UP_FM_RECEIVER
            };
            status_ = Status::Busy;
            i2c::enqueue(new i2c::Packet(
                RCKID_FM_RADIO_I2C_ADDRESS,
                sizeof(cmd),
                cmd,
                0, // no data to read
                getStatusResponse
            ));
        } else if (!value && status_ != Status::Off) {}  {
            // TODO turn off (wait for not busy)
        }
    }

    void Radio::setFrequency(uint16_t freq_10kHz) {
    }

    void Radio::seekUp() {
    }

    void Radio::seekDown() {
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
    }

    void Radio::reset() {
        gpio::outputLow(RP_PIN_RADIO_RESET);
        cpu::delayMs(10);
        gpio::setAsInputPullUp(RP_PIN_RADIO_RESET);
    }

    void Radio::getStatusResponse([[maybe_unused]] uint8_t numBytes) {
        ASSERT(instance_->status_ == Status::Busy);
        uint8_t cmd = CMD_GET_INT_STATUS;
        i2c::enqueue(new i2c::Packet(
            RCKID_FM_RADIO_I2C_ADDRESS,
            sizeof(cmd),
            &cmd,
            1,
            processStatusResponse
        ));        
    }

    void Radio::processStatusResponse(uint8_t numBytes) {
        ASSERT(numBytes == 1);
        instance_->status_ = Status::On;
        i2c::readResponse(reinterpret_cast<uint8_t*>(&instance_->response_), numBytes);
        if (instance_->response_.cts())
            instance_->status_ = Status::On;
    }





} // namespace rckid