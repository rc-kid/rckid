#pragma once

#include <rckid/rckid.h>

namespace rckid::gbcemu {

    /** Implementation of the Audio Processing Unit (APU)
     
        The implementation is provided as a standalone class so that it can in theory be used in other projects as well for simple gbclike sound generation. It provides simple interface that can be triggered from the emulator itself and is completely responsible for the audio playback independent of the emulator main loop. This greatly simplifies the design, but introduces certain latencies and mis-behaviors. However, given the nature of audio playback, those should be almost imperceptible.

        For the sound emulation, we have registers controlling the four channels (NR1x..NR4x) and global registers (NR5x) which enable/disable the APU and control volume & mixer settings.

        # Olden stuff & my comments

        ff30-ff3f is wave pattern RAM (but each pattern is just 4 bits)

        The APU ticks at 512Hz with the following changes every N ticks:

            Sound length   @2 (256Hz)
            CH1 Freq sweep @4 (128Hz)
            Env sweep      @8 (64Hz)

        So the idea to emulate this is as follows: Have sample rate at 32768Hz exactly, and have audio buffers with 512 stereo samples length. This means the audio refill will be called at 64Hz (slightly more than framerate, but we do not want to keep them tied at this time). Then during each audio callback, we will do 4 identical iterations of generating 128 samples and updating the length envelope. 
            
        The square channels will also have their waveform generated. so the generation will be the same for simplicity. The noise channel is different, we'll have to figure out what to do there later.

        ## Square channels (1 & 2)

        They are almost identical, except channel 1 has frequency sweep. We use the same implementation for both with the frequency sweep in channel 1 permanently disabled. The waveform is a square wave with 4 possible duty cycles (12.5%, 25%, 50%, 75%). The channels then support length counters and volume envelopes. 

        ### Length counter

        ### Volume envelope

        The volume is 0..15 and can be set to decrease or increase at given pace. For square channels the volume is effectively the high value of the audio wave. 

     */
    class APU {
    public:

        static constexpr uint32_t APU_STEREO_SAMPLES = 512; 
 
        class SquareChannel {
        public:
            /** Determine whether the channel will be sent to left, right, or both outputs. On GBC, this is controlled by the NR51 register.
             */
            bool enableLeft = false;
            bool enableRight = false;
            /** Initial volume after triggering (0..15). On GBC this is set set by the NRx2 register.
              */
            uint8_t initialVolume;
            uint8_t envelopeDirection;
            uint8_t envelopePace;
            uint8_t initialLength;
            uint16_t period;
            uint8_t dutyCycle;
            /** True if the channel length counting is enabled */
            bool lengthEnabled;

            bool active() const { return active_; }

            /** Triggers the channel, i.e. starts playing the desired waveform.
             */
            void trigger() {
                active_ = true;
                volume_ = initialVolume;
                volumeDir_ = envelopeDirection;
                volumeSweepPace_ = envelopePace;
                volumeSweepCounter_ = 0;
                lengthCounter_ = initialLength;
                periodCounter_ = 2048 - period;
                //sampleIndex_ = 0;
                // TODO reset frequency sweep
            }

            static constexpr uint8_t DUTY_12_5 = 1;
            static constexpr uint8_t DUTY_25 = 2;
            static constexpr uint8_t DUTY_50 = 4;
            static constexpr uint8_t DUTY_75 = 6;

        private:

            friend class APU;

            bool active_ = false;

            /** Length tick, which happens at 256Hz interval, i.e. 4 times per audio callback routine. Only used when the channel length is enabled. */
            void lengthTick() {
                if (lengthEnabled) {
                    if (lengthCounter_ < 64)
                        lengthCounter_++;
                    else
                        active_ = false;
                }
            }

            uint8_t lengthCounter_ = 0;

            /** Envelope tick, which happens at 64Hz, i.e. once per audio callback routine. Setting the pace to 0 disables the envelope. */
            void envelopeTick() {
                if (++volumeSweepCounter_ == volumeSweepPace_) {
                    volumeSweepCounter_ = 0;
                    if (volumeDir_ != 0) {
                        volume_ = volume_ + volumeDir_;
                        // clamp volume to 0..15
                        if (volume_ == 15) {
                            volumeDir_ = 0;
                        } else if (volume_ == 0) {
                            active_ = false;
                            volumeDir_ = 0;
                        }
                    }
                }
            }

            uint8_t volume_ = 0;
            int8_t volumeDir_ = 0; // +1, 0, -1
            uint8_t volumeSweepPace_ = 0;
            uint8_t volumeSweepCounter_ = 0;

            /** TODO frequency sweep tick */
            void sweepTick() {

            }

            /** Generates 128 stereo samples of the channel's waveform into the provided buffer. The waveform is added to any value already present in the buffer.
             */
            void generateWaveform(int16_t * into, APU & apu) {
                // don't do anything if not active
                if (!active_)
                    return;
                int16_t x = volume_ * 1170 / 15;
                for (uint32_t i = 0; i < 128; ++i) {
                    int16_t sample = (sampleIndex_ < dutyCycle) ? x : - x;


                    into[i * 2] += (sample * enableLeft) * apu.volumeLeft_;
                    into[i * 2 + 1] += (sample * enableRight) * apu.volumeRight_;
                    // move to next sample
                    periodCounter_ += 32;
                    while (periodCounter_ >= 2048) {
                        periodCounter_ = periodCounter_ - 2048 + period;
                        sampleIndex_ = (sampleIndex_ + 1) & 7;
                    }
                }
            }

            uint16_t periodCounter_;
            uint8_t sampleIndex_ = 0;


        }; // APU::SquareChannel

        class WaveChannel {
        public:
            bool enableLeft = false;
            bool enableRight = false;
           
            uint8_t const * waveTable = nullptr;

            uint8_t outputLevel; 
            uint8_t initialLength;
            uint16_t period;

            /** True if the channel length counting is enabled */
            bool lengthEnabled;

            bool active() const { return active_; }

            void trigger() {
                active_ = true;
                lengthCounter_ = initialLength;
                // make sure we have the right sample at start
                readCurrentSample();
            }

        private:

            friend class APU;

            bool active_ = false;

            /** Length tick, which happens at 256Hz interval, i.e. 4 times per audio callback routine. Only used when the channel length is enabled. */
            void lengthTick() {
                if (lengthEnabled) {
                    if (lengthCounter_ < 255)
                        lengthCounter_++;
                    else
                        active_ = false;
                }
            }

            uint8_t lengthCounter_ = 0;

            /** Generates 128 stereo samples of the channel's waveform into the provided buffer.  */
            void generateWaveform(int16_t * into, APU & apu) {
                // don't do anything if not active or if muted
                if (!active_ || outputLevel == 0)
                    return;
                for (uint32_t i = 0; i < 128; ++i) {
                    into[i * 2] += (lastSample_ * enableLeft) * apu.volumeLeft_;
                    into[i * 2 + 1] += (lastSample_ * enableRight) * apu.volumeRight_;
                    // move to next sample
                    periodCounter_ += 64; // wave channel period increments once per 2 dots
                    while (periodCounter_ >= 2048) {
                        periodCounter_ = periodCounter_ - 2048 + period;
                        sampleIndex_ = (sampleIndex_ + 1) & 31;
                        readCurrentSample();
                    }
                }
            }

            void readCurrentSample() {
                if (waveTable == nullptr)
                    return;
                uint8_t b = waveTable[sampleIndex_ >> 1];
                if ((sampleIndex_ & 1) == 0)
                    b = b >> 4;
                // get the stored value, apply output level shift and resize to +/- 1170 so that it matches square wave
                lastSample_ = ((b & 0x0f) >> (outputLevel - 1)) * (1170 * 2) / 16;
                lastSample_ -= 1170;
            }

            uint16_t periodCounter_;
            uint8_t sampleIndex_ = 0;
            int16_t lastSample_ = 0;

        }; // APU::WaveChannel

        /** Noise channel. 

            The noise channel is a simple LFSR where the frequency is determined by a clock divider and clock shift, while the function feedback for the LFSR is the following function:

                not (b0 xor b1)

            On the GB hardware, the LFSR is either 15, or 7 bits long, whereas the implementation is more generic and allows setting a LFSR mask that can specify exactly the bit where feedback will be written (and thus the length of the LFSR). 

            The LFSR is shifted at the following frequency:

                    262144
                ---------------
                div * 2 ^ shift

            Where divider can be 0.5, 1...7 and shift can be 0..15. To stay within integer math, we will recalculate this as:

                    524288
                --------------------
                (div *2) * 2^(shift)

         */
        class NoiseChannel {
        public:
            bool enableLeft = false;
            bool enableRight = false;
            /** Initial volume after triggering (0..15). On GBC this is set set by the NRx2 register.
              */
            uint8_t initialVolume;
            uint8_t envelopeDirection;
            uint8_t envelopePace;
            uint8_t initialLength;
            /** When enabled, the LFSR will be 7bits long instead of the default 15.
             */
            uint16_t lfsrMask; 
            /** Clock shift 0..15 
             */
            uint8_t clkShift;
            /** Clock divider 0..7, where 0 is treated as 0.5, i.e. twice the speed
             */
            uint8_t clkDiv;

            /** True if the channel length counting is enabled */
            bool lengthEnabled;

            bool active() const { return active_; }

            void trigger() {
                active_ = true;
                lengthCounter_ = initialLength;
                volume_ = initialVolume;
                volumeDir_ = envelopeDirection;
                volumeSweepPace_ = envelopePace;
                volumeSweepCounter_ = 0;
                // reset lfsr when triggered
                lfsr_ = 0;
                static constexpr int32_t dividers[] = { 1, 2, 4, 6, 8, 10, 12, 14 };
                period_ = dividers[clkDiv] * (2 ^ clkShift);
            }

        private:

            friend class APU;

            bool active_ = false;

            /** Length tick, which happens at 256Hz interval, i.e. 4 times per audio callback routine. Only used when the channel length is enabled. */
            void lengthTick() {
                if (lengthEnabled) {
                    if (lengthCounter_ < 64)
                        lengthCounter_++;
                    else
                        active_ = false;
                }
            }

            uint8_t lengthCounter_ = 0;

            /** Envelope tick, which happens at 64Hz, i.e. once per audio callback routine. Setting the pace to 0 disables the envelope. */
            void envelopeTick() {
                if (++volumeSweepCounter_ == volumeSweepPace_) {
                    volumeSweepCounter_ = 0;
                    if (volumeDir_ != 0) {
                        volume_ = volume_ + volumeDir_;
                        // clamp volume to 0..15
                        if (volume_ == 15) {
                            volumeDir_ = 0;
                        } else if (volume_ == 0) {
                            active_ = false;
                            volumeDir_ = 0;
                        }
                    }
                }
            }

            uint8_t volume_ = 0;
            int8_t volumeDir_ = 0; // +1, 0, -1
            uint8_t volumeSweepPace_ = 0;
            uint8_t volumeSweepCounter_ = 0;

            void generateWaveform(int16_t * into, APU & apu) {
                // don't do anything if not active
                if (!active_)
                    return;
                int16_t x = volume_ * 1170 / 15;
                for (uint32_t i = 0; i < 128; ++i) {
                    int16_t sample = (lfsr_ & 1) ? x : - x;
                    into[i * 2] += (sample * enableLeft) * apu.volumeLeft_;
                    into[i * 2 + 1] += (sample * enableRight) * apu.volumeRight_;
                    // determine when to shift the register
                    // TODO this is not correct we should lfsr as many times as the frequency dictates, not just once per sample - maybe not noticeable?
                    periodCounter_ -= 16;
                    if (periodCounter_ < 0) {
                        periodCounter_ = period_;
                        lfsrTick();
                    }
                }
            }

            void lfsrTick() {
                // do the b0 xor b1 and then invert the first bit and copy result to whole value so that we  can mask
                uint16_t x = (lfsr_ & 2) >> 1;
                x = x ^ (lfsr_ & 1);
                x = x ? 0x0: 0xffff;
                lfsr_ >>= 1;
                lfsr_ &= lfsrMask;
                lfsr_ |= (x & ~lfsrMask);
            }


            int32_t periodCounter_;
            int32_t period_;
            uint16_t lfsr_;

        }; // APU::NoiseChannel


        /** Getters for the channel generators to control & trigger.
         */
        SquareChannel & channel1() { return ch1_; }
        SquareChannel & channel2() { return ch2_; }
        WaveChannel & channel3() { return ch3_; }
        NoiseChannel & channel4() { return ch4_; }

        void initialize(uint8_t * waveTable) {
            ch3_.waveTable = waveTable;
        }
        /** Enables or disables the APU. 
         */
        void enable(bool value) {
            if (value == enabled_)
                return;
            if (value) {
                audioPlay(soundBuffer_, 32768, [this](int16_t * buffer, uint32_t stereoSamples){
                    memset32(reinterpret_cast<uint32_t*>(buffer), 0, APU_STEREO_SAMPLES);
                    //for (int i = 0; i < 128; ++i)
                    //    buffer[i] = 2048;
                    generateWaveform(buffer);
                    lengthTick();
                    generateWaveform(buffer + 256);
                    lengthTick();
                    sweepTick();
                    generateWaveform(buffer + 512);
                    lengthTick();
                    generateWaveform(buffer + 768);
                    lengthTick();
                    sweepTick();
                    envelopeTick();
                    return stereoSamples;
                });
            } else {
                audioStop();
            }
            enabled_ = value;
        }

        bool enabled() const { return enabled_; }

        void setVolume(uint8_t masterVolume) {
            masterVolume &= 7;
            volumeLeft_ = masterVolume;
            volumeRight_ = masterVolume;
        }

        void setVolume(uint8_t left, uint8_t right) {
            volumeLeft_ = left & 7;
            volumeRight_ = right & 7;
        }

        /** \name GB HW Interface
         
            To simpify development, the APU class also provides a simple interface to the GB memory mapped IO registers so that the emulators do not have to perform the translation from register updates to APU calls. Other users of the APU should instead call directly the explicit interface functions.
         */
        //@{
        static constexpr uint32_t ADDR_NR10 = 0x10;
        static constexpr uint32_t ADDR_NR11 = 0x11;
        static constexpr uint32_t ADDR_NR12 = 0x12;
        static constexpr uint32_t ADDR_NR13 = 0x13;
        static constexpr uint32_t ADDR_NR14 = 0x14;
        static constexpr uint32_t ADDR_NR21 = 0x16;
        static constexpr uint32_t ADDR_NR22 = 0x17;
        static constexpr uint32_t ADDR_NR23 = 0x18;
        static constexpr uint32_t ADDR_NR24 = 0x19;
        static constexpr uint32_t ADDR_NR30 = 0x1a;
        static constexpr uint32_t ADDR_NR31 = 0x1b;
        static constexpr uint32_t ADDR_NR32 = 0x1c;
        static constexpr uint32_t ADDR_NR33 = 0x1d;
        static constexpr uint32_t ADDR_NR34 = 0x1e;
        static constexpr uint32_t ADDR_NR41 = 0x20;
        static constexpr uint32_t ADDR_NR42 = 0x21;
        static constexpr uint32_t ADDR_NR43 = 0x22;
        static constexpr uint32_t ADDR_NR44 = 0x23;
        static constexpr uint32_t ADDR_NR50 = 0x24;
        static constexpr uint32_t ADDR_NR51 = 0x25;
        static constexpr uint32_t ADDR_NR52 = 0x26;
        
        void setRegister(uint32_t reg, uint8_t value, uint8_t * hram) {
            // if the APU is not enabled, all register writes are ignored, except when APU is enabled by setting MSB of NR_52
            if (!enabled_) {
                if (reg == ADDR_NR52 && (value & 0x80)) {
                    enable(true);
                    hram[ADDR_NR52] |= 0x80;
                }
                return;
            }
            // translate register updates to the interface calls
            switch (reg) {
                // channel 1 (square wave + sweep)
                case ADDR_NR10:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR10 = sweep                  |   | Pace      |Dir|  Step     |
                    // UNIMPLEMENTED;
                    break;
                case ADDR_NR11:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR11 = length & duty          | Duty  | Init length           |
                    ch1_.initialLength = value & 0x3f;
                    ch1_.dutyCycle = squareDutyFromRegister_[value >> 6];
                    break;
                case ADDR_NR12:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR12 = volume & envelope      | Init Vol      |Dir| Sweep pace|
                    ch1_.initialVolume = value >> 4;
                    ch1_.envelopeDirection = (value & 0x08) ? +1 : -1;
                    ch1_.envelopePace = value & 0x07;
                    break;
                case ADDR_NR13:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR13 = period low             | Period LSB                    |
                    ch1_.period = (ch1_.period & 0x700) | value;
                    break;
                case ADDR_NR14:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR14 = period high & ctrl     |Trg|LEn|           | Period MSB|
                    ch1_.period = (ch1_.period & 0xff) | ((value & 0x07) << 8);
                    ch1_.lengthEnabled = (value & 0x40);
                    if (value & 0x80)
                        ch1_.trigger();
                    break;
                // channel 2 (square wave)
                // case ADDR_NR20: // no sweep is available on GB for channel 2
                case ADDR_NR21:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR21 = length & duty          | Duty  | Init length           |
                    ch2_.initialLength = value & 0x3f;
                    ch2_.dutyCycle = squareDutyFromRegister_[value >> 6];
                    break;
                case ADDR_NR22:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR22 = volume & envelope      | Init Vol      |Dir| Sweep pace|
                    ch2_.initialVolume = value >> 4;
                    ch2_.envelopeDirection = (value & 0x08) ? +1 : -1;
                    ch2_.envelopePace = value & 0x07;
                    break;
                case ADDR_NR23:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR23 = period low             | Period LSB                    |
                    ch2_.period = (ch2_.period & 0x700) | value;
                    break;
                case ADDR_NR24:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR24 = period high & ctrl     |Trg|LEn|           | Period MSB|
                    ch2_.period = (ch2_.period & 0xff) | ((value & 0x07) << 8);
                    ch2_.lengthEnabled = (value & 0x40);
                    if (value & 0x80)
                        ch2_.trigger();
                    break;
                // channel 3 (wave)
                case ADDR_NR30:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR30                          | On|                           |
                    if ((value & 0x80) == 0)
                        ch3_.active_ = false;
                    break;
                case ADDR_NR31:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR31 = length                 | Init length                   |
                    ch3_.initialLength = value;
                    break;
                case ADDR_NR32:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR32 = volume                 |   | Init vol|                 |
                    ch3_.outputLevel = value >> 4 & 3;
                    break;
                case ADDR_NR33:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR33 = period low             | Period LSB                    |
                    ch3_.period = (ch3_.period & 0x1f00) | value;
                    break;
                case ADDR_NR34:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR34 = period high & ctrl     |Trg|LEn|   | Period MSB        |
                    ch3_.period = (ch3_.period & 0xff) | ((value & 0x1f) << 8);
                    ch3_.lengthEnabled = (value & 0x40);
                    if (value & 0x80)
                        ch3_.trigger();
                    break;
                // channel 4 (noise)
                case ADDR_NR41:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR41 = length                 |       | Init length           |
                    ch4_.initialLength = value & 0x3f;
                    break;
                case ADDR_NR42:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR42 = volume & envelope      | Init Vol      |Dir| Sweep pace|
                    ch4_.initialVolume = value >> 4;
                    ch4_.envelopeDirection = (value & 0x08) ? +1 : -1;
                    ch4_.envelopePace = value & 0x07;
                    break;
                case ADDR_NR43:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR43 = freq randomness        | ClkShift      |LFSR|ClkDiv    |
                    ch4_.lfsrMask = (value & 0x08) ? 0x7f : 0x7fff;
                    ch4_.clkShift = (value >> 4) & 0x0f;
                    ch4_.clkDiv = value & 0x07;
                    break;
                case ADDR_NR44:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR44 = ctrl                   |Trg|LEn|                       |
                    ch4_.lengthEnabled = (value & 0x40);
                    if (value & 0x80)
                        ch4_.trigger();
                    break;
                // global APU settings
                case ADDR_NR50:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR50 = volume & VIN           |VinL|Vol Left  |VinR|VolvRight |
                    // ignore VIn as no game ever used it
                    volumeRight_ = (value & 7); 
                    volumeLeft_ = (value >> 4 & 7);
                    break;
                case ADDR_NR51:
                    //                               | 7   6   5   4   3   2   1   0 |
                    //                               | Left          | Right         |
                    // NR51 = sound panning          |Ch4|Ch3|Ch2|Ch1|Ch4|Ch3|Ch2|Ch1|
                    ch1_.enableLeft = (value & 0x01);
                    ch2_.enableLeft = (value & 0x02);
                    ch3_.enableLeft = (value & 0x04);
                    ch4_.enableLeft = (value & 0x08);
                    ch1_.enableRight = (value & 0x10);
                    ch2_.enableRight = (value & 0x20);
                    ch3_.enableRight = (value & 0x40);
                    ch4_.enableRight = (value & 0x80);
                    break;
                case ADDR_NR52:
                    //                               | 7   6   5   4   3   2   1   0 |
                    // NR52 = enable & ch status     | On|           |Ch4|Ch3|Ch2|Ch1|
                    // only bit 7 can be changed to 0, which disables the APU
                    if ((value & 0x80) == 0) {
                        enable(false);
                        hram[ADDR_NR52] = 0;
                    }
                    break;
                default:
                    // invalid or unimplemented register
                    UNREACHABLE;
            }
            // default write to the possibly updated value to hram
            hram[reg] = value;
        }

        uint8_t getRegister(uint32_t reg, uint8_t * hram) {
            uint8_t result = hram[reg];
            switch (reg) {
                case ADDR_NR52:
                    // update the channel status bits
                    result = (result & 0x80) | (ch1_.active() ? 0x01 : 0x00) | (ch2_.active() ? 0x02 : 0x00) | (ch3_.active() ? 0x04 : 0x00) | (ch4_.active() ? 0x08 : 0x00);
                    break;
                default:
                    // all other registers are read directly from hram
                    break;
            }
            return result;
        }
        //@}

    private:

        void lengthTick() {
            ch1_.lengthTick();
            ch2_.lengthTick();
            ch3_.lengthTick();
            ch4_.lengthTick();
        }

        void envelopeTick() {
            ch1_.envelopeTick();
            ch2_.envelopeTick();
            //ch3_.envelopeTick();
            ch4_.envelopeTick();
        }

        void sweepTick() {
            ch1_.sweepTick();
            ch2_.sweepTick();
            //ch3_.sweepTick();
            //ch4_.sweepTick();
        }

        void generateWaveform(int16_t * into) {
            ch1_.generateWaveform(into, *this);
            ch2_.generateWaveform(into, *this);
            ch3_.generateWaveform(into, *this);
            ch4_.generateWaveform(into, *this);
        }

        // translation table from NR11/21 register duty bits to actual duty cycle
        static constexpr uint8_t squareDutyFromRegister_[] = {
            SquareChannel::DUTY_12_5, 
            SquareChannel::DUTY_25, 
            SquareChannel::DUTY_50, 
            SquareChannel::DUTY_75
        };

        bool enabled_ = false;

        // apu channels
        SquareChannel ch1_;
        SquareChannel ch2_;
        WaveChannel ch3_;
        NoiseChannel ch4_;

        // left and right channel volumes, from 0 to 7
        uint8_t volumeLeft_;
        uint8_t volumeRight_;

        DoubleBuffer<int16_t> soundBuffer_{1024};


    }; // gbcemu::APU


} // namespace rckid::gbcemu

