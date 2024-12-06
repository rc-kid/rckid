#pragma once

#include <platform.h>
namespace rckid::audio {

    enum class Channels {
        Mono = 1, 
        Stereo = 2,
    }; 

    enum class SampleRate {
        khz8, 
        khz12,
        khz16,
        khz24,
        khz44_1,
        khz48, 
    };

    inline void convertToStereo(int16_t * buffer, uint32_t numSamples) {
        for (; numSamples > 0; --numSamples) {
            int16_t x = buffer[numSamples];
            buffer[numSamples * 2] = x;
            buffer[numSamples * 2 + 1] = x;
        }
        buffer[1] = buffer[0];
    }

} // namespace rckid::audio