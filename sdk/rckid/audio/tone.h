#pragma once

#include "../rckid.h"
#include "../utils/interpolation.h"
#include "../utils/buffers.h"
#include "audio.h"


namespace rckid {

    /** Waveform, which is just a buffer of 16bit integers. Internally, waveform is a lazy buffer so readonly waveforms from ROM do not take up space, while dynamic waveforms are possible with the same codebase.
     */
    class Waveform : public LazyBuffer<int16_t> {
    public:
        Waveform(Allocator & a = Heap::allocator()): 
            LazyBuffer{square50_, a} {
        }

        template<size_t SIZE>
        Waveform(int16_t const (&from)[SIZE], Allocator & a = Heap::allocator()): 
            LazyBuffer{from, SIZE, a} {
        }

        static Waveform Sine(Allocator & a = Heap::allocator()) {
            return Waveform{assets::WaveformSin, a};
        }

        using LazyBuffer::LazyBuffer;

        // TODO square, sine, triangle, etc waveforms, can do white noise as well

    private:
        static constexpr int16_t square50_[] = { audio::min, audio::min, audio::min, audio::min, audio::max, audio::max, audio::max, audio::max};

    }; // rckid::Waveform

    /** A very simple tone generator. 
     
        Can generate particular waveform at given frequency. The waveform is always rendered so that for each period (1/f) the whole waveform will be used.  
        
        Internally comverts all to duration in nanoseconds using fixedInts
     */
    class Tone {
    public:

        void setWaveform(Waveform && wf) {
            wf_ = std::move(wf);
        }

        void setSampleRate(uint32_t sampleRate) {
            sr_ = audio::frequencyToPeriodUs(sampleRate);
        }

        Waveform const & waveform() const { return wf_; }

        /** Turns the tone on with given frequency.
         */
        void on(uint32_t freq) { 
            period_ = audio::frequencyToPeriodUs(freq);
        }

        /** Turns the tone off. 
         */
        void off() { 
            period_ = 0;
        }

        /** Generates the given tone into the buffer. Takes the number of samples to generate and the sample rate at which playback will occur as arguments. 
         */
        uint32_t generateInto(int16_t * buffer, uint32_t numSamples) {
            ASSERT(numSamples > 0); // at least one sample must be generated
            int16_t * end = buffer + numSamples;
            if (period_ == 0) {
                // if frequency is 0, just generate silence of appropriate length
                while (buffer != end)
                    *(buffer++) = 0;
            } else {
                // generate the waveform - sample rate tells us how much to advance at each step
                while (buffer != end) {
                    uint32_t index = interpolation::linear(t_, period_, 0, waveform().size()).round();
                    *(buffer++) = waveform()[index]; // const access only
                    t_ += sr_;
                    if (t_ >= period_)
                        t_ -= period_;
                }
            }
            return numSamples;
        }

    protected:
        // period in milliseconds
        FixedInt period_{0};
        // current position in the period
        FixedInt t_{0};

        // sample rate (period)
        FixedInt sr_;
        Waveform wf_;
    }; 

} // namespace rckid