#pragma once

#include <platform.h>

namespace rckid {

    /** LSM6DSV Accelerometer, gyroscope & pedometer driver.
     

        Official ST driver (which I found after I wrote the below code:) https://github.com/STMicroelectronics/lsm6dsv-pid

    */
    class LSM6DSV : public i2c::Device {
    public: 
        static constexpr uint8_t I2C_ADDRESS = 0x6A;

        /** Accelerometer scale supported by the sensor.
         */
        enum class AccelScale : uint8_t {
            G2 = 0,
            G4 = 1,
            G8 = 2,
            G16 = 3,
        };

        PACKED(struct Orientation3D {
            int16_t x;
            int16_t y;
            int16_t z;
        });

        static_assert(sizeof(Orientation3D) == 6);

        /** Creates the accelerometer driver. Does not perform any communication with the chip, for this use the initialize() method below. 
         */
        LSM6DSV(): i2c::Device{I2C_ADDRESS} {}

        bool initialize() {
            // enable block data update and auto increment (those are default values anyways)
            i2c::writeRegister<uint8_t>(address, REG_CTRL3, CTRL3_BDU | CTRL3_IF_INC);
            // disable FIFO and set to bypass mode
            i2c::writeRegister<uint8_t>(address, REG_FIFO_CTRL4, 0x00);
            // disable accelerometer and gyroscope by setting ODR to power down, high performance mode
            i2c::writeRegister<uint8_t>(address, REG_CTRL1, 0x00);
            // disable gyroscope by setting ODR to power down, high performance mode
            i2c::writeRegister<uint8_t>(address, REG_CTRL2, 0x00);
            // set accelerometer scale to 2G scale (required by the pedometer)
            setAccelerometerScale(AccelScale::G2);
            return true;
        }

        /** Performs software triggered power-on reset of the acdelerometer. 
         
            Note that aside of the SW_POR there is also boot bit and sw reset bits in CTRL_3 that seem to do some resetting as well.
         */
        void reset() {
            i2c::writeRegister<uint8_t>(address, REG_FUNC_CFG_ACCESS, FUNC_CFG_ACCESS_SW_POR);
            cpu::delayMs(30);
        }

        /** Gets the device ID, (WHO AM I register), which is supposed to be fixed at 0x70. Can be used to verify communiation.
         */
        uint8_t getDeviceId() {
            return i2c::readRegister<uint8_t>(address, REG_WHO_AM_I);
        }

        /** Enables or disables the accelerometer. The accelerometer runs at 30Hz when enabled as this value is required by the pedometer.
         */
        void enableAccelerometer(bool value) {
            i2c::writeRegister<uint8_t>(address, REG_CTRL1, value ? ODR_30HZ : 0x00);
        }

        bool isAccelerometerEnabled() {
            return (i2c::readRegister<uint8_t>(address, REG_CTRL1) & 0b1111) != 0;
        }

        AccelScale getAccelerometerScale() {
            return static_cast<AccelScale>(i2c::readRegister<uint8_t>(address, REG_CTRL8) & 0b11);
        } 

        void setAccelerometerScale(AccelScale scale) {
            i2c::writeRegister<uint8_t>(address, REG_CTRL8, static_cast<uint8_t>(scale));
        }

        Orientation3D readAccelerometerRaw() {
            uint8_t reg = REG_OUTX_L_A;
            Orientation3D result;
            i2c_write_blocking(i2c0, I2C_ADDRESS, & reg, 1, true);
            i2c_read_blocking(i2c0, I2C_ADDRESS, reinterpret_cast<uint8_t *>(& result), sizeof(result), false);
            //lsm6dsv16x_acceleration_raw_get(&reg_ctx, reinterpret_cast<int16_t *>(& result));
            return result;
        }

        void enableGyroscope(bool value) {

        }

        Orientation3D readGyroscopeRaw() {
            uint8_t reg = REG_OUTX_L_G;
            Orientation3D result;
            i2c_write_blocking(i2c0, I2C_ADDRESS, & reg, 1, true);
            i2c_read_blocking(i2c0, I2C_ADDRESS, reinterpret_cast<uint8_t *>(& result), sizeof(result), false);
            return result;
        }

        /** Enables, or disables the pedometer function. 
         
            The pedometer requires the accelerometer to run at 30Hz and 2G sensitivity scale. 
         */
        void enablePedometer(bool value) {
            // switch to embedded function registers
            i2c::writeRegister<uint8_t>(address, REG_FUNC_CFG_ACCESS, FUNC_CFG_ACCESS_EMB);
            // enable pedometer function
            i2c::writeRegister<uint8_t>(address, REG_EMB_FUNC_EN_A, value ? EMB_FUNC_EN_A_PEDO : 0x00);
            // enable interrupt routing to INT2 (unused)
            i2c::writeRegister<uint8_t>(address, REG_EMB_FUNC_INT2, value ? EMB_FUNC_INT_PEDO : 0x00);
            // reinitialize the algorithm
            if (value)
                i2c::writeRegister<uint8_t>(address, REG_EMB_FUNC_INIT_A, EMB_FUNC_INIT_A_PEDO);
            // go back to normal registers
            i2c::writeRegister<uint8_t>(address, REG_FUNC_CFG_ACCESS, 0x00);
        }

        bool isPedometerEnabled() {
            // switch to embedded function registers
            i2c::writeRegister<uint8_t>(address, REG_FUNC_CFG_ACCESS, FUNC_CFG_ACCESS_EMB);
            uint8_t x = i2c::readRegister<uint8_t>(address, REG_EMB_FUNC_EN_A);
            // go back to normal registers
            i2c::writeRegister<uint8_t>(address, REG_FUNC_CFG_ACCESS, 0x00);
            return (x & EMB_FUNC_EN_A_PEDO);
        }

        uint16_t readStepCount() {
            i2c::writeRegister<uint8_t>(address, REG_FUNC_CFG_ACCESS, FUNC_CFG_ACCESS_EMB);
            uint16_t count = i2c::readRegister<uint16_t>(address, REG_EMB_FUNC_STEP_COUNTER_L);
            // go back to normal registers
            i2c::writeRegister<uint8_t>(address, REG_FUNC_CFG_ACCESS, 0x00);
            return count;
        }

        void resetStepCount() {
            // switch to embedded function registers
            i2c::writeRegister<uint8_t>(address, REG_FUNC_CFG_ACCESS, FUNC_CFG_ACCESS_EMB);
            // set the reset bit for step count
            i2c::writeRegister<uint8_t>(address, REG_EMB_FUNC_SRC, EMB_FUNC_SRC_STEP_RESET);
            // go back to normal registers
            i2c::writeRegister<uint8_t>(address, REG_FUNC_CFG_ACCESS, 0x00);
        }

    private:

    public:
        static constexpr uint8_t REG_FUNC_CFG_ACCESS = 0x01;
        static constexpr uint8_t FUNC_CFG_ACCESS_EMB = 0x80;
        static constexpr uint8_t FUNC_CFG_ACCESS_SW_POR = 0b00000100;

        static constexpr uint8_t REG_FIFO_CTRL4 = 0x0a; // FIFO mode


        static constexpr uint8_t REG_WHO_AM_I = 0x0f;

        static constexpr uint8_t REG_CTRL1 = 0x10; // accelerometer mode & ODR
        static constexpr uint8_t ODR_30HZ =  0b0100; // ODR at 30Hz 
        static constexpr uint8_t REG_CTRL2 = 0x11; // gyroscope mode & ODR


        static constexpr uint8_t REG_CTRL3 = 0x12; // block data update, auto increment
        static constexpr uint8_t CTRL3_BDU      = 0b01000000;
        static constexpr uint8_t CTRL3_IF_INC   = 0b00000100;
        static constexpr uint8_t CTRL3_SW_RESET = 0b00000001;

        static constexpr uint8_t REG_CTRL8 = 0x17; // accelerometer scale, low pass filter


        static constexpr uint8_t REG_OUTX_L_G = 0x22; // gyroscope output (3x int16_t)
        static constexpr uint8_t REG_OUTX_H_G = 0x23;
        static constexpr uint8_t REG_OUTY_L_G = 0x24;
        static constexpr uint8_t REG_OUTY_H_G = 0x25;
        static constexpr uint8_t REG_OUTZ_L_G = 0x26;
        static constexpr uint8_t REG_OUTZ_H_G = 0x27;

        static constexpr uint8_t REG_OUTX_L_A = 0x28; // accelerometer output (3x int16_t)
        static constexpr uint8_t REG_OUTX_H_A = 0x29;
        static constexpr uint8_t REG_OUTY_L_A = 0x2A;
        static constexpr uint8_t REG_OUTY_H_A = 0x2B;
        static constexpr uint8_t REG_OUTZ_L_A = 0x2C;
        static constexpr uint8_t REG_OUTZ_H_A = 0x2D;



        static constexpr uint8_t REG_EMB_FUNC_EN_A = 0x04;
        static constexpr uint8_t EMB_FUNC_EN_A_PEDO = 0b00001000;

        static constexpr uint8_t REG_EMB_FUNC_INT2 = 0x0e;
        static constexpr uint8_t EMB_FUNC_INT_PEDO = 0b00001000;

        static constexpr uint8_t REG_EMB_FUNC_STEP_COUNTER_L = 0x62;
        static constexpr uint8_t REG_EMB_FUNC_STEP_COUNTER_H = 0x63;

        static constexpr uint8_t REG_EMB_FUNC_SRC = 0x64;
        static constexpr uint8_t EMB_FUNC_SRC_STEP_RESET = 0x80;

        static constexpr uint8_t REG_EMB_FUNC_INIT_A = 0x66;
        static constexpr uint8_t EMB_FUNC_INIT_A_PEDO = 0b00001000;

    }; // rckid::LSM6DSV

} // namespace rckid