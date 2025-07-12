#pragma once

#include "i2s_mclk.pio.h"
#include "i2s_out16.pio.h"


#include <platform.h>
#include <rckid/log.h>
//#include "../i2c.h"


namespace rckid {

    class Codec {
    public:

        static void initialize() {
            // initialize the PIO, which we use for MCLK and I2S communication
            // we use pio1 as the I2S pins are in the lower bank (pio0 is occupied by the display)
            // TODO can this be shared instead, i.e. can we move those up, or maybe just the mclk generation?
            pio_set_gpio_base(pio1, 0);
            mclkSm_ = pio_claim_unused_sm(pio1, true);
            mclkOffset_ = pio_add_program(pio1, & i2s_mclk_program);
            LOG(LL_INFO, "      sm: " << (uint32_t)mclkSm_);
            LOG(LL_INFO, "  offset: " << (uint32_t)mclkOffset_);
        }

        /** Enables the MCLK generation for given sample rate. 
         
            As per the datasheet, page 48, the IMCLK must be exactly 256x the sample rate
         */
        static void enableMasterClock(uint32_t sampleRate) {
            uint32_t hz = sampleRate * 256 * 2; // two instructions per cycle
            LOG(LL_INFO, "Enabling MCLK at " << hz << "Hz");
            pio_sm_set_enabled(pio1, mclkSm_, false); // disable the SM first
            i2s_mclk_program_init(pio1, mclkSm_, mclkOffset_, RP_PIN_I2S_MCLK);
            pio_set_clock_speed(pio1, mclkSm_, hz);
            pio_sm_set_enabled(pio1, mclkSm_, true); // and finally enable 
        }

        static void reset() {
            LOG(LL_INFO, "Resetting codec");
            setRegister(REG_RESET, 0xff);
            cpu::delayMs(10);
            LOG(LL_INFO, "  rev " << hex(getRegister(REG_DEV_REVISION)));
            LOG(LL_INFO, "   id " << hex(getRegister(REG_DEV_ID)));
        }

        /** Powers the codec up.
         
            This follows the datasheet, page 20, Power up guidance and configures the device for speaker and headphones output with below 3.6V operation. The sequence here should prevent speaker pop & click. 
         */
        static void powerUp() {
            // enable thermal shutdown and disable boosts (ideal for < 3.6V operation)
            setRegister(REG_OUTPUT_CONTROL, TSEN);
            // enable tie off buffer
            setRegister(REG_PWR_MGMT_1, IOBUFEN);
            // allow the ref voltage to charge slowly by the internal coupling resistors
            setRegister(REG_PWR_MGMT_1, REF_IMP_80K | IOBUFEN | ABIASEN);
            // wait 250ms for the charging to complete
            cpu::delayMs(250);
            // now enable the outputs (but keep them muted)
            setRegister(REG_PWR_MGMT_2, RHPEN | LHPEN);
            setRegister(REG_PWR_MGMT_3, RSPKEN | LSPKEN);
            // ensure that we use mclk and not the pio (MCLK direct, no PLL, slave mode, no MCLK divider)
            setRegister(REG_CLK_CTRL_1, 0); 


            // for right spaker enable direct connection from rmix to spkr bypassing the submixer
            setRegister(REG_RSPKR_SUBMIX, RMIXMUT | RSUBBYP);
        }

        static void powerDown() {

        }




        /** Sets the speaker volume, 0..63. When 0 is set instead of the lowest volume supported  
         * 
         */
        static void setSpeakerVolume(uint8_t volume) {
            if (volume == 0) {
                setRegister(REG_LSPKOUT_VOLUME, SPEAKER_MUTE);
                setRegister(REG_RSPKOUT_VOLUME, SPEAKER_MUTE | UPDATE_BIT);
            } else {
                setRegister(REG_LSPKOUT_VOLUME, volume);
                setRegister(REG_RSPKOUT_VOLUME, volume | UPDATE_BIT);
            }
        }

        static void setHeadphonesVolume(uint8_t volume) {
            if (volume == 0) {
                setRegister(REG_LHP_VOLUME, HEADPHONE_MUTE);
                setRegister(REG_RHP_VOLUME, HEADPHONE_MUTE | UPDATE_BIT);
            } else {
                setRegister(REG_LHP_VOLUME, volume);
                setRegister(REG_RHP_VOLUME, volume | UPDATE_BIT);
            }
        }

        /** Configures the codec for simple playback from the aux line in inputs.
         
            We can't use the normal line in inputs that go through the PGA as they are also used as the headphone jack detection.    
          */
        static void playbackLineInDirect() {
            // enable the ADC mix/boost stages (and heep the headphone drivers enabled, but muted)
            setRegister(REG_PWR_MGMT_2, RHPEN | LHPEN | RBSTEN | LBSTEN);
            // enable the RMIX and LMIX sections (keep the speaker on as well)
            setRegister(REG_PWR_MGMT_3, RSPKEN | LSPKEN | RMIXEN | LMIXEN);
            // connect the AUX inputs to the ADC boost section
            setRegister(REG_LADC_BOOST, 0b111); // 0db, use 111 for max 6db 
            setRegister(REG_RADC_BOOST, 0b111);
            // and connect the ADC boost section to the output mixers
            setRegister(REG_LMIXER, 0b11110);
            setRegister(REG_RMIXER, 0b11110);

            // try the spkr boost

        }


        static void showRegisters() {
            LOG(LL_INFO, "Codec registers:");
            LOG(LL_INFO, "   1: " <<  hex(getRegister(1)) << " exp: 0x000d");
            LOG(LL_INFO, "   2: " <<  hex(getRegister(2)) << " exp: 0x01b0");
            LOG(LL_INFO, "   3: " <<  hex(getRegister(3)) << " exp: 0x006c"); 
            LOG(LL_INFO, "  43: " << hex(getRegister(43)) << " exp: 0x0000"); // might invert the polarity for BTL
            LOG(LL_INFO, "  47: " << hex(getRegister(47)) << " exp: 0x0005");
            LOG(LL_INFO, "  48: " << hex(getRegister(48)) << " exp: 0x0005");
            LOG(LL_INFO, "  49: " << hex(getRegister(49)) << " exp: 0x0002");
            LOG(LL_INFO, "  50: " << hex(getRegister(50)) << " exp: 0x0016");
            LOG(LL_INFO, "  51: " << hex(getRegister(51)) << " exp: 0x0016");
            LOG(LL_INFO, "  52: " << hex(getRegister(52)) << " exp: 0x007f");
            LOG(LL_INFO, "  53: " << hex(getRegister(53)) << " exp: 0x007f");
            LOG(LL_INFO, "  54: " << hex(getRegister(54)) << " exp: 0x007f");
            LOG(LL_INFO, "  55: " << hex(getRegister(55)) << " exp: 0x007f");

/*
LL_INFO: Codec registers
:
LL_INFO:    1: 0x000d
LL_INFO:    2: 0x01b0
LL_INFO:    3: 0x01b0
LL_INFO:   43: 0x0020
LL_INFO:   49: 0x0002
LL_INFO:   50: 0x0002
LL_INFO:   51: 0x0002
LL_INFO:   52: 0x007f
LL_INFO:   53: 0x007f
LL_INFO:   54: 0x0039
LL_INFO:   55: 0x0039


LL_INFO: Codec registers
:
LL_INFO:    1: 0x0000 exp: 0x000d
LL_INFO:    2: 0x01b0 exp: 0x01b0
LL_INFO:    3: 0x006c exp: 0x006c
LL_INFO:   43: 0x0020 exp: 0x0000
LL_INFO:   47: 0x0005 exp: 0x0005
LL_INFO:   48: 0x0005 exp: 0x0005
LL_INFO:   49: 0x0002 exp: 0x0002
LL_INFO:   50: 0x0016 exp: 0x0016
LL_INFO:   51: 0x0016 exp: 0x0016
LL_INFO:   52: 0x003f exp: 0x007f
LL_INFO:   53: 0x003f exp: 0x007f
LL_INFO:   54: 0x003f exp: 0x007f
LL_INFO:   55: 0x003f exp: 0x007f

*/            
        }

    private:

        static constexpr uint16_t UPDATE_BIT = 256;

        static constexpr uint8_t REG_RESET = 0;

        static constexpr uint8_t REG_PWR_MGMT_1 = 1;
        static constexpr uint16_t REF_IMP_80K = 0x01;
        static constexpr uint16_t IOBUFEN = 4;
        static constexpr uint16_t ABIASEN = 8;

        static constexpr uint8_t REG_PWR_MGMT_2 = 2;
        static constexpr uint16_t RHPEN = 256;
        static constexpr uint16_t LHPEN = 128;
        static constexpr uint16_t RBSTEN = 32;
        static constexpr uint16_t LBSTEN = 16;

        static constexpr uint8_t REG_PWR_MGMT_3 = 3;
        static constexpr uint16_t LSPKEN = 64;
        static constexpr uint16_t RSPKEN = 32;
        static constexpr uint16_t RMIXEN = 8;
        static constexpr uint16_t LMIXEN = 4;

        static constexpr uint8_t REG_AUDIO_INTERFACE = 4;
        static constexpr uint8_t REG_COMPANDING = 5;
        static constexpr uint8_t REG_CLK_CTRL_1 = 6;
        static constexpr uint8_t REG_CLK_CTRL_2 = 7;
        static constexpr uint8_t REG_GPIO = 8;
        static constexpr uint8_t REG_JACK_DETECT_1 = 9;
        static constexpr uint8_t REG_DAC_CTRL = 10;
        static constexpr uint8_t REG_LDAC_VOLUME = 11;
        static constexpr uint8_t REG_RDAC_VOLUME = 12;
        static constexpr uint8_t REG_JACK_DETECT_2 = 13;
        static constexpr uint8_t REG_ADC_CTRL = 14;
        static constexpr uint8_t REG_LADC_VOLUME = 15;
        static constexpr uint8_t REG_RADC_VOLUME = 16;
        // no 17
        static constexpr uint8_t REG_EQ1_CUTOFF_LOW = 18;
        static constexpr uint8_t REG_EQ2_PEAK_1 = 19;
        static constexpr uint8_t REG_EQ3_PEAK_2 = 20;
        static constexpr uint8_t REG_EQ4_PEAK_3 = 21;
        static constexpr uint8_t REG_EQ5_CUTOFF_HIGH = 22;
        // no 23
        static constexpr uint8_t REG_DAC_LIMITER_1 = 24;
        static constexpr uint8_t REG_DAC_LIMITER_2 = 25;
        // no 26
        static constexpr uint8_t REG_NOTCH_FILTER_1 = 27;
        static constexpr uint8_t REG_NOTCH_FILTER_2 = 28;
        static constexpr uint8_t REG_NOTCH_FILTER_3 = 29;
        static constexpr uint8_t REG_NOTCH_FILTER_4 = 30;
        // no 31
        static constexpr uint8_t REG_ALC_CTRL_1 = 32;
        static constexpr uint8_t REG_ALC_CTRL_2 = 33;
        static constexpr uint8_t REG_ALC_CTRL_3 = 34;
        static constexpr uint8_t REG_NOISE_GATE = 35;
        static constexpr uint8_t REG_PLL_N = 36;
        static constexpr uint8_t REG_PLL_K1 = 37;
        static constexpr uint8_t REG_PLL_K2 = 38;
        static constexpr uint8_t REG_PLL_K3 = 39;
        // no 40
        static constexpr uint8_t REG_3D_CTRL = 41;
        // no 42
        static constexpr uint8_t REG_RSPKR_SUBMIX = 43;
        static constexpr uint16_t RMIXMUT = 0b100000;
        static constexpr uint16_t RSUBBYP = 0b010000;

        static constexpr uint8_t REG_INPUT_CTRL = 44;
        static constexpr uint8_t REG_LINPUT_PGA_GAIN = 45;
        static constexpr uint8_t REG_RINPUT_PGA_GAIN = 46;
        static constexpr uint8_t REG_LADC_BOOST = 47 ;
        static constexpr uint8_t REG_RADC_BOOST = 48;

        static constexpr uint8_t REG_OUTPUT_CONTROL = 49;
        static constexpr uint16_t TSEN  = 2;

        static constexpr uint8_t REG_LMIXER = 50;
        static constexpr uint8_t REG_RMIXER = 51;


        static constexpr uint8_t REG_LHP_VOLUME = 52;
        static constexpr uint8_t REG_RHP_VOLUME = 53;
        static constexpr uint16_t HEADPHONE_MUTE = 64;

        static constexpr uint8_t REG_LSPKOUT_VOLUME = 54;
        static constexpr uint8_t REG_RSPKOUT_VOLUME = 55;
        static constexpr uint16_t SPEAKER_MUTE = 64;

        static constexpr uint8_t REG_AUX2_MIXER = 56;
        static constexpr uint8_t REG_AUX1_MIXER = 57;
        static constexpr uint8_t REG_PWR_MGMT_4 = 58;
        static constexpr uint8_t REG_LTIMESLOT = 59;
        static constexpr uint8_t REG_MISC = 60;
        static constexpr uint8_t REG_RTIMESLOT = 61;

        static constexpr uint8_t REG_DEV_REVISION = 62;
        static constexpr uint8_t REG_DEV_ID = 63;

        // no 64
        static constexpr uint8_t REG_DAC_DITHER = 65;
        // no 66-68
        static constexpr uint8_t REG_5V_BIASING = 69;
        static constexpr uint8_t REG_ALC_ENHANCE_1 = 70;
        static constexpr uint8_t REG_ALC_ENHANCE_2 = 71;
        static constexpr uint8_t REG_192KHZ_SAMPLING = 72;
        static constexpr uint8_t REG_MISC_CTRL = 73;
        static constexpr uint8_t REG_INPUT_TIEOFF = 74;
        // no 75
        static constexpr uint8_t REG_AGC_PTP_READOUT = 76;
        static constexpr uint8_t REG_ARC_PDETECT_READOUT = 77;
        static constexpr uint8_t REG_AUTOMUTE = 78;
        static constexpr uint8_t REG_OUTPUT_TIEOFF = 79;
        // no 80
        static constexpr uint8_t REG_PWR_REDUCTION_TIEOFF = 81;
        // rest - not used
        static constexpr uint8_t REG_SPI1 = 87;
        static constexpr uint8_t REG_SPI2 = 108;
        static constexpr uint8_t REG_SPI3 = 115;

        // TODO switch to the async i2c interface
        static void setRegister(uint8_t reg, uint16_t value) {
            uint8_t cmd[] = {
                static_cast<uint8_t>((reg << 1) | ((value >> 8) & 0x01)),
                static_cast<uint8_t>(value & 0xff)
            };
            ::i2c::masterTransmit(RCKID_AUDIO_CODEC_I2C_ADDRESS, cmd, 2, nullptr, 0);
        }

        static uint16_t getRegister(uint8_t reg) {
            uint8_t result[2];
            reg <<= 1;
            ::i2c::masterTransmit(RCKID_AUDIO_CODEC_I2C_ADDRESS, & reg, 1, result, 2);
            return (result[0] << 8) | result[1];;
        }

        static inline int mclkSm_ = -1;
        static inline uint mclkOffset_ = 0;

    }; // rckid::Codec

} // namespace rckid