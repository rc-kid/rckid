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

// 37391
        static constexpr uint16_t Forever = 0;

        using GENERATOR::valueAt;

        Tone():
            delta_{WavePeriod / (sampleRate_ * 10)} { // because frequency is x10, we have adjust sample rate as well 
        }

        uint16_t nextValue(uint16_t amp) {
            uint16_t result = valueAt(frequency_ * acc_, amp);
            acc_ = (acc_ + delta_) % period_;
            return result;
        }

        void fillBuffer(uint16_t * buffer, size_t bufferSize) override {
            if (duration_ != 0) {
                uint16_t * bufferEnd = buffer + bufferSize;
                while (buffer != bufferEnd) {
                    uint16_t v = nextValue(255);
                    *(buffer++) = v;
                    *(buffer++) = v;
                    if (--duration_ == 0)
                        onDone();
                }
            } else if (frequency_ != 0) {
                uint16_t * bufferEnd = buffer + bufferSize;
                while (buffer != bufferEnd) {
                    uint16_t v = nextValue(255);
                    *(buffer++) = v;
                    *(buffer++) = v;
                }
            } else {
                memset(buffer, 0, bufferSize * 2);
            }
        };

        /** Sets the output frequency and duration. Frequency is expected in Hz times 10 (i.e. 4400 for A) and duration is expected in microseconds. 
         */
        void setFrequency(unsigned hzx10, unsigned duration = Forever) {
            frequency_ = hzx10;
            acc_ = 0;
            if (frequency_ != 0)
                period_ = WavePeriod / frequency_;
            // convert duration to number of ticks at given sample rate
            if (duration == Forever)
                duration_ = 0;
            else 
                duration_ = duration * 10 / (10000000 / sampleRate_);
        }

    protected:

        /** Called when the tone's duration has been exhausted (or never in case of forever tones). Overriding in children allows reacting to tone being finished. The default implementation simply turns the tone off. 
         */
        virtual void onDone() {
            setFrequency(0);
        }

    private:

        // frequency of the tone generated x10
        uint16_t frequency_ = 0;
        // period of the frequency in ticks (uses entire uint32_t range)
        uint32_t period_ = 0;
        // period accumulator 
        uint32_t acc_ = 0;
        // accumulator and duration increase per sample rate tick
        uint32_t delta_ = 0; 
        // duration of the tone in ticks
        uint32_t duration_ = 0;

    }; // rckid::Tone


    /** Tone with an ADSR envelope
     */
    template<typename GENERATOR>
    class ADSRTone {

    }; // rckid::ADSR

} // namespace rckid