#pragma once

#include <cstdint>

namespace rckid::assets {

    /** One quarter of sine wave, from 0 to max within uint16_t.
     */
    constexpr int16_t SineTable[] = {
        #include "interpolation_sine.inc.h"
    };

    constexpr int16_t SineMax = 32767;

} // namespace rckid
