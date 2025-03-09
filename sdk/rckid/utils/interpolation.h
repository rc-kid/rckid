#pragma once

#include "fixedint.h"
#include "timer.h"
#include "../assets/sine_table.h"

/** Interpolation functions. 

    TODO do other interpolations
 */
namespace rckid::interpolation {

    inline int custom(FixedInt i, FixedInt period, int16_t const * data, uint32_t length) {
        return data[((i * static_cast<int>(length - 1)) / period).round()];    
    }

    inline FixedInt linear(FixedInt i, FixedInt period, int min, int max) {
        return FixedInt{min} + (i * (max - min) / period);
    }

    inline FixedInt linear(Timer const & t, int min, int max) {
        return linear(t.t(), t.duration(), min, max);
    }

    /** Cosine easing function. Starts slow, ramps up to top speed in the middle and then slows down again using the cos function. Useful for menu shifting, etc. 
     */
    inline FixedInt cosine(Timer const & t, int min, int max) {
        int hwDiv =  32767 * 2; // sine max * 2 
        FixedInt p2 = t.duration() / 2;
        FixedInt i = t.t();
        if (i <= p2)
            return min + (max - min) / 2 - custom(p2 - i, p2, assets::SineTable, sizeof(assets::SineTable) / 2) * (max - min) / hwDiv;
        i -= p2;
        return min + (max - min) / 2 + custom(i, p2, assets::SineTable, sizeof(assets::SineTable) / 2) * (max - min) / hwDiv;
    }

} // namespace rckid::interpolation