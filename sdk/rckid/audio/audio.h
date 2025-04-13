#pragma once

#include "../rckid.h"
#include "../utils/fixedint.h"

namespace rckid {

    /** Given a frequency, returns the corresponding period in microseconds (us) as a fixed integer.
      */
    inline FixedInt frequencyToPeriodUs(uint32_t freq) {
        freq = 1000000 * 16 / freq;
        return FixedInt{static_cast<int>(freq >> 4), freq & 0xf};
    }



} // namespace rckid