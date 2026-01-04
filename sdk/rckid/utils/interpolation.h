#pragma once

#include "fixedint.h"
#include "timer.h"
#include "../assets/waveforms.h"

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

    /** Cosine easing function. Starts slow, ramps up to top speed in the middle and then slows down again using the cos function. Useful for menu shifting, etc. Can be cheaply calculated using the middle portion of the sine wave and distance from the max int16_t value. 
     */
    inline FixedInt cosine(Timer const & t, int min, int max) {
        FixedInt i = t.t();
        FixedInt period = t.duration();
        uint32_t size = sizeof(assets::WaveformSin) / sizeof(int16_t);
        int value = custom(i, period, assets::WaveformSin + size / 4, size / 2) + 32768;
        auto x = min + FixedInt{(65536 - value) * (max - min)} / 65535;
        return x; 
        //return FixedInt{65536 - value} * (max - min) / period;
    }

    inline FixedInt cosineLoop(Timer const & t, int min, int max) {
        FixedInt i = t.t();
        FixedInt period = t.duration();
        FixedInt halfPeriod = period / 2;
        uint32_t size = sizeof(assets::WaveformSin) / sizeof(int16_t);
        int value;
        if (i < halfPeriod) {
            value = custom(i, halfPeriod, assets::WaveformSin + size / 4, size / 2) + 32768;
        } else {
            value = custom(i - halfPeriod, halfPeriod, assets::WaveformSin + size / 4, size / 2) + 32768;
        }
        auto x = min + FixedInt{(65536 - value) * (max - min)} / 65535;
        return x; 
    }

    /**  
     */
    inline FixedInt cosineNudge(Timer const & t, int min, int max) {
        FixedInt i = t.t();
        FixedInt period = t.duration();
        FixedInt halfPeriod = period / 2;
        uint32_t size = sizeof(assets::WaveformSin) / sizeof(int16_t);
        int value = 1;
        if (i < halfPeriod) {
            value = custom(i, halfPeriod, assets::WaveformSin + size / 4, size / 2) + 32768;
            return min + FixedInt{(65536 - value) * (max - min)} / 65535;
        } else {
            value = custom(i - halfPeriod, halfPeriod, assets::WaveformSin + size / 4, size / 2) + 32768;
            return max - FixedInt{(65536 - value) * (max - min)} / 65535;
        }
    }

} // namespace rckid::interpolation