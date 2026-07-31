#include <rckid/capabilities/light_sensor.h>

namespace rckid {

    LightSensor lightSensor_;

    LightSensor * LightSensor::instance() {
        return & lightSensor_;
    }

    uint8_t LightSensor::ambientLight() {
        // just a test
        return 128;
    }

} // namespace rckid