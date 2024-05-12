#pragma once

#include "rckid/rckid.h"
#include "audio_stream.h"
#include "wave_generators.h"

namespace rckid {

    /** Simple tone generator. 

        Generates a tone with given frequency based on the provided generator.  
     */
    template<typename GENERATOR>
    class Tone : public AudioStream, public GENERATOR {
    public:

        using GENERATOR::valueAt;

        void fillBuffer(uint16_t * buffer, size_t bufferSize) override {
            if (frequency_ == 0) {
                memset(buffer, 0, bufferSize * 2);
            } else {
                // figure out our delta based on the sample rate
                uint32_t delta = WavePeriod / (sampleRate_ * 10);
                uint16_t * bufferEnd = buffer + bufferSize;
                while (buffer < bufferEnd) {
                    uint16_t v = valueAt(frequency_ * acc_, );
                    *(buffer++) = v;
                    *(buffer++) = v;
                    acc_ = (acc_ + delta) % period_;
                }
            }
        };

        void setFrequency(unsigned hzx10) {
            frequency_ = hzx10;
            acc_ = 0;
            if (frequency_ != 0)
                period_ = WavePeriod / frequency_;
        }

        void setSampleRate(SampleRate rate) override {
            AudioStream::setSampleRate(rate);
        }

    private:

        uint16_t frequency_ = 0;
        uint32_t period_ = 0;
        uint32_t acc_ = 0;

    }; // rckid::Tone

} // namespace rckid