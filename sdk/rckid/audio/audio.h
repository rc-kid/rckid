#pragma once

#include "../rckid.h"
#include "../utils/fixedint.h"

namespace rckid {

    namespace audio {

        constexpr int16_t min = -32768;
        constexpr int16_t max = 32767;

    } // namespace audio

    /** Given a frequency, returns the corresponding period in microseconds (us) as a fixed integer.
      */
    inline FixedInt frequencyToPeriodUs(uint32_t freq) {
        freq = 1000000 * 16 / freq;
        return FixedInt{static_cast<int>(freq >> 4), freq & 0xf};
    }



} // namespace rckid