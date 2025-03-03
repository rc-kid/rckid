#pragma once

#include "fixedint.h"
#include "timer.h"

/** Interpolation functions. 

    TODO do other interpolations
 */
namespace rckid::interpolation {

        inline FixedInt linear(FixedInt i, FixedInt period, int min, int max) {
            return FixedInt{min} + (i * (max - min) / period);
        }

        inline FixedInt linear(Timer const & t, int min, int max) {
            return linear(t.t(), t.duration(), min, max);
        }

} // namespace rckid::interpolation