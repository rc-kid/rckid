#pragma once

#include "rckid/rckid.h"

namespace rckid {

    enum class SampleRate : uint16_t {
        kHz8 = 8000, 
        kHz16 = 16000,
        kHz44_1 = 44100,
        kHz48 = 48000,
    };

    constexpr uint16_t AudioBaseLevel = 2048;

    class AudioStream {
    public:

        uint16_t sampleRate() const { return sampleRate_; }

        virtual void fillBuffer(uint16_t * buffer, size_t bufferSize) = 0;

    protected:

        uint16_t sampleRate_ = 44100;

    }; // rckid::AudioStream


} // namespace rckid