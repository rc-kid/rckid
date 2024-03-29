#pragma once

namespace rckid {

    class BMI160 {
    public:

        struct State {
            int16_t gyroX;
            int16_t gyroY;
            int16_t gyroZ;
            int16_t accelX;
            int16_t accelY;
            int16_t accelZ;
        } __attribute__((packed)); 

        static constexpr uint8_t I2C_ADDRESS = 0x68;

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

        static bool isPresent();

        static void initialize();

        static void measure(State & state);
        
    }; // rckid::BMI160

    class LTR390UV {
    public:


        static constexpr uint8_t I2C_ADDRESS = 0x53;

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


        static bool isPresent();

        static void initialize();

        static void startALS();
        static uint16_t measureALS();

        static void startUV();
        static uint16_t measureUV();

    }; // rckid::LTR390UV

} // namespace rckid 