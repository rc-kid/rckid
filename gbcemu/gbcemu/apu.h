#pragma once

#include <rckid/rckid.h>

namespace rckid::gbcemu {

    /** Implementation of the Audio Processing Unit (APU)
     
        The implementation is provided as a standalone class so that it can in theory be used in other projects as well for simple gbclike sound generation. It provides simple interface that can be triggered from the emulator itself and is completely responsible for the audio playback independent of the emulator main loop. This greatly simplifies the design, but introduces certain latencies and mis-behaviors. However, given the nature of audio playback, those should be almost imperceptible.

        For the sound emulation, we have registers controlling the four channels (NR1x..NR4x) and global registers (NR5x) which enable/disable the APU and control volume & mixer settings.

        NR52 = APU on/off & channel status (writing to the channel status bits has no effect)
        NR51 = which channel goes to which output (left/right)
        NR50 = master volume & VIN panning (no need to use, no game used it - extra audio input from cartridge)
                                   | 7   6   5   4   3   2   1   0 |
        NR10 = sweep                  |   | Pace      |Dir|  Step     |
        (no NR11, channel 2 lacks sweep)
        NR30                          | On|                           |
        
        NR11 = length & duty          | Duty  | Init length           |
        NR21 = length & duty          | Duty  | Init length           |
        NR31 = length                 | Init length                   |
        NR41 = length                 |       | Init length           |
        
        NR12 = volume & envelope      | Init Vol      |Dir| Sweep pace|
        NR22 = volume & envelope      | Init Vol      |Dir| Sweep pace|
        NR32 = volume                 |   | Init vol|                 |
        NR42 = volume & envelope      | Init Vol      |Dir| Sweep pace|
        
        NR13 = period low             | Period LSB                    |
        NR23 = period low             | Period LSB                    |
        NR33 = period low             | Period LSB                    |
        NR43 = freq randomness        | ClkShift      |LFSR|ClkDiv    |
        
        NR14 = period high & ctrl     |Trg|LEn|   | Period MSB        |
        NR24 = period high & ctrl     |Trg|LEn|   | Period MSB        |
        NR34 = period high & ctrl     |Trg|LEn|   | Period MSB        |
        NR44 = ctrl                   |Trg|LEn|                       |

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
            //bool lengthEnabled;

            /** Triggers the channel, i.e. starts playing the desired waveform.
             */
            void trigger() {

            }

            static constexpr uint8_t DUTY_12_5 = 1;
            static constexpr uint8_t DUTY_25 = 2;
            static constexpr uint8_t DUTY_50 = 4;
            static constexpr uint8_t DUTY_75 = 6;

        private:

            friend class APU;

            /** Length tick, which happens at 256Hz interval, i.e. 4 times per audio callback routine. Only used when the channel length is enabled. */
            void lengthTick() {
                if (lengthCounter_ < 64)
                    lengthCounter_++;
            }

            uint8_t lengthCounter_ = 0;

            /** Envelope tick, which happens at 64Hz, i.e. once per audio callback routine. Setting the pace to 0 disables the envelope. */
            void envelopeTick() {
                if (++volumeSweepCounter_ == volumeSweepPace_) {
                    volumeSweepCounter_ = 0;
                    if (volumeDir_ != 0) {
                        volume_ = volume_ + volumeDir_;
                        // clamp volume to 0..15
                        if (volume_ == 0 || volume_ == 15)
                            volumeDir_ = 0;
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
                // TODO don't do anything if not active
                for (uint32_t i = 0; i < 128; ++i) {
                    int16_t sample = (sampleIndex_ < dutyCycle) ? volume_ : - volume_;


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

        }; // APU::WaveChannel

        class NoiseChannel {

        }; // APU::NoiseChannel


        /** Getters for the channel generators to control & trigger.
         */
        SquareChannel & channel1() { return ch1_; }
        SquareChannel & channel2() { return ch2_; }
        WaveChannel & channel3() { return ch3_; }
        NoiseChannel & channel4() { return ch4_; }

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
                    ch1_.generateWaveform(buffer, *this);
                    ch2_.generateWaveform(buffer, *this);
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
                case ADDR_NR52:
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
            // TODO
            return 0;
        }
        //@}

    private:
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

