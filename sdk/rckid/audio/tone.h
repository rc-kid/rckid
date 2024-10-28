#pragma once

#include <platform/buffer.h>

#include "../rckid.h"
#include "../utils/fixedint.h"
#include "../utils/interpolation.h"

namespace rckid {

    class Tone;

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

        uint32_t attackMs() const { return attackMs_; }
        uint32_t decayMs() const { return decayMs_; }
        uint32_t sustainPct() const { return sustainPct_; }
        uint32_t releaseMs() const { return releaseMs_; }

        void setAttackMs(uint32_t value) { 
            attackMs_ = value; 
        }

        void setDecayMs(uint32_t value) {
            decayMs_ = value;
        }

        void setSustainPct(uint32_t value) {
            ASSERT(value <= 100);
        }

        void setReleaseMs(uint32_t value) {
            releaseMs_ = value;
        }

        void setEnvelope(uint32_t attackMs, uint32_t decayMs, uint32_t sustainPct, uint32_t releaseMs) {
            attackMs_ = attackMs;
            decayMs_ = decayMs;
            sustainPct_ = sustainPct;
            releaseMs_ = releaseMs;
            updateEnvelope();
        }

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
            di_ = 0;
            if (frequency != 0)
                period_ = FixedInt{static_cast<int>(sampleRate)} / static_cast<int>(frequency);
            else
                period_ = 0;
            duration_ = sampleRate * durationMs / 1000;
            releaseStart_ = (releaseMs_ < durationMs) ? (duration_ - releaseMs_ * sampleRate / 1000) : duration_;     
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
            di_ = 0;
            if (onDone_)
                onDone_(*this);
        }

        /** Returns the next value of the waveform based on the tone waveform itself and the selected freqency.
         */
        int16_t next() {
            if (duration_ == 0)
                return 0;
            if (++di_ == duration_) {
                off();
                return 0;
            }
            // just a silence note
            if (period_ == 0)
                return 0;
            // get current index
            FixedInt i = i_;
            // move to next index
            i_ += 1;
            if (i_ > period_)
                i_ -= period_;
            // update the amplitude based on the envelope
            int amp = sustainAmp_;
            if (di_ >= releaseStart_)
                amp = interpolation::linear(di_ - releaseStart_, duration_ - releaseStart_, amp, 0);
            else if (di_ < attackEnd_)
                amp = interpolation::linear(di_, attackEnd_, 0, MAX);
            else if (di_ < decayEnd_)
                amp = interpolation::linear(di_ - attackEnd_, decayEnd_ - attackEnd_, MAX, sustainAmp_);
            // and calculate the waveform
            switch (waveform_) {
                // simple square wave with 50% duty cycle 
                case Waveform::Square:
                    return interpolation::square(i, period_, -amp, amp);
                // sawtooth - MIN to MAX then MAX to MIN
                case Waveform::Sawtooth:
                    return interpolation::sawtooth(i, period_, -amp, amp);
                // triangle from MIN to MAX
                case Waveform::Triangle:
                    return interpolation::linear(i, period_, -amp, amp);
                // sine wave from the sine interpolator 
                case Waveform::Sine:
                    return interpolation::sine(i, period_, -amp, amp);
                // custom waveform from the waveform buffer
                case Waveform::Custom:
                    return interpolation::custom(i, period_, waveformData_, waveformLength_);
                default:
                    UNREACHABLE;
            }
        }

    private:

        void updateEnvelope() {
            attackEnd_ = attackMs_ * audioSampleRate() / 1000;
            decayEnd_ = attackEnd_ + decayMs_ * audioSampleRate() / 1000;
            sustainAmp_ = MAX * sustainPct_ / 100;
        }

        static constexpr int16_t MIN = -8191;
        static constexpr int16_t MAX = 8191;

        FixedInt period_;
        FixedInt i_;
        int32_t di_;
        int32_t duration_;

        // envelope
        uint32_t attackMs_;
        uint32_t decayMs_;
        uint32_t releaseMs_;
        uint32_t sustainPct_;

        // envelope updated to the given frequency
        int32_t attackEnd_ = 0;
        int32_t decayEnd_ = 0;
        int32_t releaseStart_ = 0;
        int sustainAmp_ = MAX;

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