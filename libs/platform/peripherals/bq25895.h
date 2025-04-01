#pragma once

#include "platform.h"

namespace platform {

    /** BQ25895 
     */
    class BQ25895 : public i2c::Device {
    public:

        BQ25895(uint8_t address): i2c::Device{address} {}


    }; // platform::BQ25895

} // namespace platform