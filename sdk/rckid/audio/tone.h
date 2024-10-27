#pragma once

#include <platform/buffer.h>

#include "../rckid.h"
#include "../utils/fixedint.h"
#include "../utils/interpolation.h"

namespace rckid {

    /** A simple ADSR envelope.
     
        Provides the timings of the attack, decay, sustain and release 
        
     */
    class Envelope {
    public:
        Envelope(uint32_t attack, uint32_t decay, uint32_t sustain, uint32_t release):
            attack_{attack}, decay_{decay}, sustain_{sustain}, release_{release} {}

    private:
        uint32_t attack_;
        uint32_t decay_;
        uint32_t sustain_;
        uint32_t release_;

    }; // rckid::Envelope

    /** Simple tone generator. 
     
        Allows generating tone of given frequency, sample rate, waveform and envelope.  
     */
    class Tone {
    public:

        using OnDone = std::function<void(Tone &)>;

        enum class Waveform {
            Square, 
            Sawtooth, 
            Triangle,
            Sine,
            Custom,
        }; 

        void setWaveform(Waveform wf) {
            ASSERT(wf != Waveform::Custom); // use setCustomWaveform instead
            waveform_ = wf;
        }

        void setCustomWaveform(int16_t const * waveForm, int32_t length) {
            waveform_ = Waveform::Custom;
            waveformData_ = waveForm;
            waveformLength_ = length;
        }

        void clearOnDone() { onDone_ = nullptr; }

        void setOnDone(OnDone cb) { onDone_ = cb; }

        /** Sets note frequency and sample rate. 
         */
        void setFrequency(uint32_t frequency, uint32_t durationMs, uint32_t sampleRate) {
            i_ = 0;
            if (frequency != 0)
                period_ = FixedInt{static_cast<int>(sampleRate)} / static_cast<int>(frequency);
            else
                period_ = 0;
            duration_ = sampleRate * durationMs / 1000;
            TRACE_TONE("Tone frequency " << frequency << ", period: " << period_.clip(), " duration ticks " << duration_);
        }

        void setFrequency(uint32_t frequency, uint32_t durationMs = std::numeric_limits<uint32_t>::max()) {
            setFrequency(frequency, durationMs, audioSampleRate());
        }

        void off() {
            TRACE_TONE("Tone off");
            period_ = 0;
            duration_ = 0;
            i_ = 0;
            if (onDone_)
                onDone_(*this);
        }

        /** Returns the next value of the waveform based on the tone waveform itself and the selected freqency.
         */
        int16_t next() {
            if (duration_ == 0)
                return 0;
            // get current index
            FixedInt i = i_;
            if (--duration_ == 0) {
                off();
                return 0;
            }
            // just a silence note
            if (period_ == 0)
                return 0;
            // move to next index
            i_ += 1;
            if (i_ > period_)
                i_ -= period_;
            switch (waveform_) {
                // simple square wave with 50% duty cycle 
                case Waveform::Square:
                    return interpolation::square(i, period_, MIN, MAX);
                // sawtooth - MIN to MAX then MAX to MIN
                case Waveform::Sawtooth:
                    return interpolation::sawtooth(i, period_, MIN, MAX);
                // triangle from MIN to MAX
                case Waveform::Triangle:
                    return interpolation::linear(i, period_, MIN, MAX);
                // sine wave from the sine interpolator 
                case Waveform::Sine:
                    return interpolation::sine(i, period_, MIN, MAX);
                // custom waveform from the waveform buffer
                case Waveform::Custom:
                    return interpolation::custom(i, period_, waveformData_, waveformLength_);
                default:
                    UNREACHABLE;
            }
        }

    private:

        static constexpr int16_t MIN = -8191;
        static constexpr int16_t MAX = 8191;

        FixedInt period_;
        FixedInt i_;
        uint32_t duration_;

        // what waveform the tone generator uses
        Waveform waveform_ = Waveform::Square;
        // for custom waveform, the waveform data for single period
        int16_t const * waveformData_;
        int32_t waveformLength_; 

        OnDone onDone_;

    }; 

    /** A simple four channel tone generator (monophonic)
     */
    class ToneGenerator {
    public:

        ToneGenerator():
            buf_{BUFFER_FRAMES * 4, [this](DoubleBuffer &) { refill(); }} {
        }


        void enable() {
            refill();
            audioPlay(buf_, f_);
        }

        void disable() {
            audioStop();
        }

        Tone & operator[] (uint32_t channel) { 
            ASSERT(channel < 4);
            return channels_[channel];
        }

    private:

        static constexpr uint32_t BUFFER_FRAMES = 512;

        // go over all the channels and refill them
        void refill() {
            int16_t * buf = reinterpret_cast<int16_t*>(buf_.getBackBuffer());
            for (uint32_t i = 0; i < BUFFER_FRAMES; ++i) {
                int16_t v = channels_[0].next();
                v += channels_[1].next();
                v += channels_[2].next();
                v += channels_[3].next();
                buf[i * 2] = v;
                buf[i * 2 + 1] = v;
            }
        }

        Tone channels_[4];
        DoubleBuffer buf_;
        uint32_t f_ = 44100;
    }; 

} // namespace rckid


/*

    What I need - multiple channels, each one is tone. Way to control actions 



*/