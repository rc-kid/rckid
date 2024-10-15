#pragma once

#include "../rckid.h"
#include "../utils/fixedint.h"

namespace rckid {

    class Envelope {
    public:



    }; // rckid::Envelope


    /** Simple tone generator. 
     
        Allows generating tone of given frequency, sample rate, waveform and envelope.  
     */
    class Tone {
    public:

        static Tone square() {
            Tone result;
            result.setWaveForm(square_, sizeof(square_) / 2);
            return result;
        }

        void setWaveForm(int16_t const * waveForm, int32_t length) {
            waveForm_ = waveForm;
            waveFormLength_ = length;
        }

        /** Sets note frequency and sample rate. 
         */
        void setFrequency(uint32_t frequency, uint32_t durationMs, uint32_t sampleRate) {
            i_ = 0;
            period_ = FixedInt{static_cast<int32_t>(sampleRate)} / static_cast<int32_t>(frequency);
            duration_ = sampleRate * durationMs / 1000;
            TRACE_TONE("Tone frequency " << frequency << ", period: " << period_.clip(), " duration ticks " << duration_);
        }

        void setFrequency(uint32_t frequency, uint32_t durationMs = std::numeric_limits<uint32_t>::max()) {
            setFrequency(frequency, durationMs, audioSampleRate());
        }

        void off() {
            TRACE_TONE("Tone off");
            period_ = 0;
            i_ = 0;
        }

        /** Returns the next value of the waveform based on the tone waveform itself and the selected freqency.
         */
        int16_t next() {
            if (period_ == 0)
                return 0;
            int16_t result = waveForm_[((i_ * waveFormLength_) / period_).clip()];
            i_ += 1;
            if (i_ > period_)
                i_ -= period_;
            if (--duration_ == 0)
                off();
            return result;
        }

    private:

        FixedInt period_;
        FixedInt i_;
        uint32_t duration_;

        int16_t const * waveForm_;
        int32_t waveFormLength_; 

        static constexpr int16_t square_[] = {-8191, 8191};
    }; 


} // namespace rckid