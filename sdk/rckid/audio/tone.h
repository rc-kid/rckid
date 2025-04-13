#pragma once

#include "../rckid.h"
#include "../utils/interpolation.h"
#include "audio.h"


namespace rckid {

    /** Simple class that holds a waveform. 
     
        The waveform, like string
     */
    class Waveform {

    }; 

    /** A very simple tone generator. 
     
        Can generate particular waveform at given frequency. The waveform is always rendered so that for each period (1/f) the whole waveform will be used.  
        
        Internally comverts all to duration in nanoseconds using fixedInts
     */
    class Tone {
    public:
        /** Turns the tone on with given frequency.
         */
        void on(uint32_t freq) { 
            period_ = frequencyToPeriodUs(freq);
        }

        /** Turns the tone off. 
         */
        void off() { 
            period_ = 0;
        }

        /** Generates the given tone into the buffer. Takes the number of samples to generate and the sample rate at which playback will occur as arguments. 
         */
        void generateInto(int16_t * buffer, uint32_t numSamples, uint32_t sampleRate) {
            ASSERT(numSamples > 0); // at least one sample must be generated
            if (f_ == 0) {
                // if frequency is 0, just generate silence of appropriate length
                while (numSamples-- > 0)
                    *(buffer++) = 0;
            } else {
                // generate the waveform - sample rate tells us how much to advance at each step
                FixedInt sr = frequencyToPeriodUs(sampleRate);
                while (numSamples-- > 0) {
                    uint32_t index = interpolation::linear(t_, period_, 0, waveformLength_);
                    *(buffer++) = waveform_[index];
                    t_ += sr;
                    if (t_ >= period_)
                        t_ -= period_;
                }
            }
        }

    protected:
        // period in milliseconds
        FixedInt period_;
        // 
        FixedInt t_;

        uint32_t waveformLength_;
        int16_t const * waveform_;
    }; 

} // namespace rckid