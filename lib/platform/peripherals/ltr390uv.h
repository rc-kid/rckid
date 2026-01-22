#pragma once

#include "platform.h"


class LTR390UV : public i2c::Device {
public:


    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x53;

    static constexpr uint8_t REG_CTRL = 0x00;
    static constexpr uint8_t CTRL_RESET = 0x10;
    static constexpr uint8_t CTRL_UV_EN = 0x0a;
    static constexpr uint8_t CTRL_ALS_EN = 0x02;

    static constexpr uint8_t REG_MEAS_RATE = 0x04;
    static constexpr uint8_t MEAS_RATE_16bit_25ms = 0b01000000;

    static constexpr uint8_t REG_GAIN = 0x05;
    static constexpr uint8_t GAIN_3 = 0x01;
    static constexpr uint8_t GAIN_6 = 0x02;
    static constexpr uint8_t GAIN_9 = 0x03;
    static constexpr uint8_t GAIN_18 = 0x04;

    static constexpr uint8_t REG_PART_ID = 0x06;
    // the reg can also contain revision number in the lower nibble
    static constexpr uint8_t PART_ID = 0b10110000;

    static constexpr uint8_t REG_STATUS = 0x07;

    static constexpr uint8_t REG_DATA_ALS = 0x0d;
    static constexpr uint8_t REG_DATA_UV = 0x10;

    LTR390UV(uint8_t address = DEFAULT_I2C_ADDRESS): i2c::Device{address} {}

    bool isPresent() {
        if (!i2c::isPresent(address))
            return false;
        return (i2c::readRegister<uint8_t>(address, REG_PART_ID) & 0xf0) == PART_ID;
    }

    void initialize() {
        i2c::writeRegister<uint8_t>(address, REG_MEAS_RATE, MEAS_RATE_16bit_25ms);
        i2c::writeRegister<uint8_t>(address, REG_GAIN, GAIN_18);
    }

    void startALS() {
        i2c::writeRegister<uint8_t>(address, REG_CTRL, CTRL_ALS_EN);
    }

    uint16_t measureALS() {
        uint8_t data[3];
        i2c::readRegister(address, REG_DATA_ALS, data, 3);
        return data[0] + data[1] * 256;
    }

    void startUV() {
        i2c::writeRegister<uint8_t>(address, REG_CTRL, CTRL_UV_EN);
    }

    uint16_t measureUV() {
        uint8_t data[3];
        i2c::readRegister(address, REG_DATA_UV, data, 3);
        return data[0] + data[1] * 256;
    }

}; // LTR390UV

