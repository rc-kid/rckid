#pragma once

#include <platform.h>

class BMI160 : public i2c::Device {
public:

    struct State {
        int16_t gyroX = 0;
        int16_t gyroY = 0;
        int16_t gyroZ = 0;
        int16_t accelX = 0;
        int16_t accelY = 0;
        int16_t accelZ = 0;
    } __attribute__((packed)); 

    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x68;

    static constexpr uint8_t REG_CHIP_ID = 0x00;
    static constexpr uint8_t CHIP_ID = 0xd1;
    static constexpr uint8_t REG_ERR = 0x02;
    static constexpr uint8_t REG_PMU_STATUS = 0x03;
    static constexpr uint8_t REG_DATA = 0x04 + 8; // ignore magnetometer data

    static constexpr uint8_t REG_MAG_IF_0 = 0x4b;
    static constexpr uint8_t REG_MAG_IF_1 = 0x4c;

    static constexpr uint8_t REG_CMD = 0x7e;
    static constexpr uint8_t CMD_ACCEL_ON = 0x11;
    static constexpr uint8_t CMD_GYRO_ON = 0x15;

    BMI160(uint8_t address = DEFAULT_I2C_ADDRESS): i2c::Device{address} {}

    bool isPresent() {
        if (!i2c::isPresent(address))
            return false;
        return i2c::readRegister<uint8_t>(address, REG_CHIP_ID) == CHIP_ID;
    }

    void initialize() {
        // TODO the initialization does not work atm for magnetometer, maybe it needs to be initialized first using the manual interface?
        i2c::writeRegister<uint8_t>(address, REG_CMD, CMD_ACCEL_ON);
        cpu::delayMs(5);
        i2c::writeRegister<uint8_t>(address, REG_CMD, CMD_GYRO_ON);
        cpu::delayMs(90);
    }

    void measure(State & state) {
        i2c::readRegister(address, REG_DATA, reinterpret_cast<uint8_t *>(& state), sizeof(State));
    }
    
}; // BMI160
