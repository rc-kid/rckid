#pragma once

#include "platform.h"


/** LSM6DSV Accelerometer, gyroscope & pedometer driver.
 
    This looks like a beast of a sensor. However there is an ST driver available on the internet that seems to provide very low level access to the functionality so that we can build a more highlevel driver on top of that for now.

    See here: https://github.com/stm32duino/LSM6DSV16X/tree/main

 */
class LSM6DSV : public i2c::Device {
public: 

    struct Orientation3D {
        int16_t x;
        int16_t y;
        int16_t z;
    };

    void initialize() {

    }

    bool isPresent() {
        if (!i2c::isPresent(address))
            return false;
        /*
        uint8_t whoami = i2c::readRegister<uint8_t>(address, 0x0f);
        return whoami == 0x6c;
        */
    }

    void enableAccelerometer(bool value) {

    }

    void enableGyroscope(bool value) {

    }

    void enablePedometer(bool value) {

    }

    Orientation3D readAccelerometer() {

    }

    Orientation3D readGyroscope() {

    }

    uint16_t readStepCount() {

    }

    void resetStepCount() {

    }

}; // LSM6DSV