#pragma once

#include "fixedint.h"
#include "../assets/interpolation_tables.h"

namespace rckid::interpolation {

    inline int custom(FixedInt i, FixedInt period, int16_t const * data, uint32_t length) {
        return data[static_cast<int>((i * static_cast<int>(length - 1)) / period)];    
    }

    template<uint32_t SIZE>
    inline int custom(FixedInt i, FixedInt period, int16_t const (&buffer)[SIZE]) {
        return custom(i, period, buffer, SIZE);
    }

    inline int square(FixedInt i, FixedInt period, int min, int max) {
        return  (i * 2 < period) ? max : min;
    }

    inline int linear(FixedInt i, FixedInt period, int min, int max) {
        return min + static_cast<int>(i * (max - min) / period);
    }

    inline int sawtooth(FixedInt i, FixedInt period, int min, int max) {
        FixedInt pHalf = period / 2;
        if (i <= pHalf) {
            return min + static_cast<int>(i * (max - min) / pHalf);
        } else {
            i -= pHalf;
            return max - static_cast<int>(i * (max - min) / pHalf);
        }
    }

    inline int sine(FixedInt i, FixedInt period, int min, int max) {
        int hwDiv = 2 * assets::SineMax; 
        FixedInt p4 = period / 4;
        if (i <= p4)
            return min + (max - min) / 2 + custom(i, p4, assets::SineTable) * (max - min) / hwDiv;
        i -= p4;
        if (i <= p4)
            return min + (max - min) / 2 + custom(p4 - i, p4, assets::SineTable) * (max - min) / hwDiv;
        i -= p4;
        if (i < p4)
            return min + (max - min) / 2 - custom(i, p4, assets::SineTable) * (max - min) / hwDiv;
        i -= p4;
        return min + (max - min) / 2 - custom(p4 -i, p4, assets::SineTable) * (max - min) / hwDiv;
    }

    inline int halfSine(FixedInt i, FixedInt period, int min, int max) {
        int hwDiv =  assets::SineMax; 
        FixedInt p2 = period / 2;
        if (i <= p2)
            return min + custom(i, p2, assets::SineTable) * (max - min) / hwDiv;
        i -= p2;
        return min + custom(p2 - i, p2, assets::SineTable) * (max - min) / hwDiv;
    }

    /** Cosine easing function. Starts slow, ramps up to top speed in the middle and then slows down again using the cos function. Useful for menu shifting, etc. 
     */
    inline int easingCos(FixedInt i, FixedInt period, int min, int max) {
        int hwDiv =  assets::SineMax * 2; 
        FixedInt p2 = period / 2;
        if (i <= p2)
            return min + (max - min) / 2 - custom(p2 - i, p2, assets::SineTable) * (max - min) / hwDiv;
        i -= p2;
        return min + (max - min) / 2 + custom(i, p2, assets::SineTable) * (max - min) / hwDiv;
    }

    inline int easingCos(Timer const & a, int min, int max) {
        return easingCos(FixedInt{a.t()}, FixedInt{a.duration()}, min, max);
    }

} // namespace rckid
 