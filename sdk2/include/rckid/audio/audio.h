#pragma once

#include <platform.h>

#include <rckid/task.h>

namespace rckid::audio {

    /** Converts mono buffer into stereo one, duplicating all samples in it in place.
     
        Takes audio buffer and number of mono samples stored in it and expands the mono samples to stereo ones, duplicating each mono sample int two stereo samples in place. The caller must ensure the buffer is large enough to hold the resulting stereo samples. 
     */
    inline void convertToStereo(int16_t * buffer, uint32_t numSamples) {
        for (; numSamples > 0; --numSamples) {
            int16_t x = buffer[numSamples - 1];
            buffer[numSamples * 2 - 2] = x;
            buffer[numSamples * 2 - 1] = x;
        }
    }

    /** Audio playback task. 
     */
    class Playback : public Task {

    }; // rckid::audio::Playback


} // namespace rckid::audio