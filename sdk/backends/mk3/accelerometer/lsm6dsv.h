#pragma once

#include "platform.h"

#include "lsm6dsv16x_reg.h"

namespace rckid {

    /** LSM6DSV Accelerometer, gyroscope & pedometer driver.
     
        This looks like a beast of a sensor. However there is an ST driver available on the internet that seems to provide very low level access to the functionality so that we can build a more highlevel driver on top of that for now.

        See here: https://github.com/stm32duino/LSM6DSV16X/tree/main

    */
    class LSM6DSV {
    public: 
        static constexpr uint8_t I2C_ADDRESS = 0x6A;

        /** Accelerometer scale supported by the sensor.
         */
        enum class AccelScale {
            G2 = LSM6DSV16X_2g,
            G4 = LSM6DSV16X_4g,
            G8 = LSM6DSV16X_8g,
            G16 = LSM6DSV16X_16g,
        };

        struct Orientation3D {
            int16_t x;
            int16_t y;
            int16_t z;
        };

        /** Creates the accelerometer driver. Does not perform any communication with the chip, for this use the initialize() method below. 
         */
        LSM6DSV() {
            reg_ctx.handle = this;
            reg_ctx.write_reg = [](void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) -> int32_t {
                uint8_t * buffer = new uint8_t [len + 1];
                buffer[0] = reg;
                memcpy(& buffer[1], bufp, len);
                i2c_write_blocking(i2c0, I2C_ADDRESS, buffer, len + 1, false);
                delete [] buffer;
                return 0;
            };
            reg_ctx.read_reg = [](void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) -> int32_t {
                i2c_read_blocking(i2c0, I2C_ADDRESS, & reg, 1, true);
                i2c_read_blocking(i2c0, I2C_ADDRESS, bufp, len, false);
                return 0;
            };
            reg_ctx.mdelay = [](uint32_t ms) { 
                cpu::delayMs(ms);
            };
        }

        bool initialize() {
            // enable register address auto increment during multiple byte access
            if (lsm6dsv16x_auto_increment_set(&reg_ctx, PROPERTY_ENABLE) != LSM6DSV16X_OK)
                return false;
            // enable block data update
            if (lsm6dsv16x_block_data_update_set(&reg_ctx, PROPERTY_ENABLE) != LSM6DSV16X_OK)
                return false;            
            // bypass FIFO
            if (lsm6dsv16x_fifo_mode_set(&reg_ctx, LSM6DSV16X_BYPASS_MODE) != LSM6DSV16X_OK)
                return false;
            // set output data rate to power down
            if (lsm6dsv16x_xl_data_rate_set(&reg_ctx, LSM6DSV16X_ODR_OFF) != LSM6DSV16X_OK)
                return false;
            if (lsm6dsv16x_gy_data_rate_set(&reg_ctx, LSM6DSV16X_ODR_OFF) != LSM6DSV16X_OK)
                return false;
            // set accelerometer scale to full 16G
            setAccelerometerScale(AccelScale::G16);
            return true;
        }

        void enableAccelerometer(bool value) {
            lsm6dsv16x_xl_data_rate_set(&reg_ctx, value ? LSM6DSV16X_ODR_AT_120Hz : LSM6DSV16X_ODR_OFF);
        }

        AccelScale getAccelerometerScale() {
            lsm6dsv16x_xl_full_scale_t scale;
            if (lsm6dsv16x_xl_full_scale_get(&reg_ctx, &scale) != LSM6DSV16X_OK)
              return static_cast<AccelScale>(0xff);
            return static_cast<AccelScale>(scale);
        }

        void setAccelerometerScale(AccelScale scale) {
            lsm6dsv16x_xl_full_scale_t lsmScale = static_cast<lsm6dsv16x_xl_full_scale_t>(scale);
            lsm6dsv16x_xl_full_scale_set(&reg_ctx, lsmScale);
        }

        Orientation3D readAccelerometerRaw() {
            Orientation3D result;
            lsm6dsv16x_acceleration_raw_get(&reg_ctx, reinterpret_cast<int16_t *>(& result));
            return result;
        }

        void enableGyroscope(bool value) {

        }

        void enablePedometer(bool value) {

        }


        Orientation3D readGyroscope() {

        }

        uint16_t readStepCount() {

        }

        void resetStepCount() {

        }

    private:

        static constexpr int32_t LSM6DSV16X_OK = 0;
        static constexpr int32_t LSM6DSV16X_ERROR = -1;

        lsm6dsv16x_ctx_t reg_ctx;

    }; // rckid::LSM6DSV

} // namespace rckid