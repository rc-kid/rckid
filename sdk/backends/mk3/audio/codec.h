#pragma once

#include "i2s_mclk.pio.h"
#include "i2s_out16.pio.h"
#include "i2s_in16.pio.h"

#include <platform.h>
#include <rckid/log.h>
#include "../i2c.h"


namespace rckid {

    /** NAU88C22GY Audio Codec driver
     

        Jack Detection  

        GPIO2 to be used (or 3). Slow clock must be enabled for the jack detection. 

        R8, R9, R13

     */
    class Codec {
    public:

        static void initialize() {
            // initialize the PIO, which we use for MCLK and I2S communication
            // we use pio1 as the I2S pins are in the lower bank (pio0 is occupied by the display)
            // TODO can this be shared instead, i.e. can we move those up, or maybe just the mclk generation?
            pio_set_gpio_base(pio1, 0);

            mclkSm_ = pio_claim_unused_sm(pio1, true);
            mclkOffset_ = pio_add_program(pio1, & i2s_mclk_program);
            LOG(LL_INFO, "      mclk sm: " << (uint32_t)mclkSm_);
            LOG(LL_INFO, "  mclk offset: " << (uint32_t)mclkOffset_);
            i2s_mclk_program_init(pio1, mclkSm_, mclkOffset_, RP_PIN_I2S_MCLK);

            playbackSm_ = pio_claim_unused_sm(pio1, true);
            playbackOffset_ = pio_add_program(pio1, & i2s_out16_program);
            LOG(LL_INFO, "      play sm: " << (uint32_t)playbackSm_);
            LOG(LL_INFO, "  play offset: " << (uint32_t)playbackOffset_);
            //i2s_out16_program_init(pio1, playbackSm_, playbackOffset_, RP_PIN_I2S_DOUT, RP_PIN_I2S_LRCK);

            recordSm_ = pio_claim_unused_sm(pio1, true);
            recordOffset_ = pio_add_program(pio1, & i2s_in16_program);
            LOG(LL_INFO, "       rec sm: " << (uint32_t)recordSm_);
            LOG(LL_INFO, "   rec offset: " << (uint32_t)recordOffset_);
        }

        static uint playbackDReq() {
            return pio_get_dreq(pio1, playbackSm_, true);
        }

        static io_wo_32 * playbackTxFifo() {
            return & pio1->txf[playbackSm_];
        }

        static uint recordDReq() {
            return pio_get_dreq(pio1, recordSm_, false);
        }

        static io_ro_32 * recordRxFifo() {
            return & pio1->rxf[recordSm_];
        }

        /** Enables the MCLK generation for given sample rate. 
         
            As per the datasheet, page 48, the IMCLK must be exactly 256x the sample rate
         */
        static void enableMasterClock(uint32_t sampleRate) {
            uint32_t hz = sampleRate * 256 * 2; // two instructions per cycle
            LOG(LL_INFO, "Enabling MCLK at " << hz << "Hz");
            pio_sm_set_enabled(pio1, mclkSm_, false); // disable the SM first
            pio_sm_set_clock_speed(pio1, mclkSm_, hz);
            pio_sm_set_enabled(pio1, mclkSm_, true); // and finally enable 
            // TODO set the sample rate for filters
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
            setRegister(REG_OUTPUT_CONTROL, TSEN /* | SPKBST */);
            // enable tie off buffer
            setRegister(REG_PWR_MGMT_1, IOBUFEN);
            // allow the ref voltage to charge slowly by the internal coupling resistors
            setRegister(REG_PWR_MGMT_1, REF_IMP_80K | IOBUFEN | ABIASEN);
            // wait 250ms for the charging to complete
            cpu::delayMs(250);
            // now enable the outputs (but keep them muted)
            setRegister(REG_PWR_MGMT_2, RHPEN | LHPEN);
            setRegister(REG_PWR_MGMT_3, RSPKEN | LSPKEN);
            // ensure that we use mclk and not the pll (MCLK direct, no PLL, slave mode, no MCLK divider)
            setRegister(REG_CLK_CTRL_1, CLKM_MCLK); 


            // for right spaker enable the inverted output so that we have BTL speaker driver
            setRegister(REG_RSPKR_SUBMIX, RSUBBYP);

            // set audio interface to 16bit I2S
            setRegister(REG_AUDIO_INTERFACE, WLEN_16 | AIFMT_I2S);

            // enable slow clock (necessary for the jack detection)
            setRegister(REG_CLK_CTRL_2, SCLKEN);
            // enable the audio jack detection or GPIO2 and automatic headphone & speaker switching, corresponding to high (pulled up) on no jack and low (connected to ground through 220k resistor) on jack connected
            // TODO do not do this yet, want to see headphones for testing purposes
            setRegister(REG_JACK_DETECT_1, JCKDEN | JCKDIO2);
            setRegister(REG_JACK_DETECT_2, JCKDOEN1_SPEAKER | JCKDOEN0_HEADPHONES);
        }

        static void powerDown() {

        }

        /** Sets the speaker volume, 0..63. When 0 is set instead of the lowest volume supported, the output is muted. Note that the speaker output is also dependent on the headphone jack state in general.   
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

        /** Stops all the audio processing. 
         */
        static void stop() {
            LOG(LL_INFO, "Codec stop");
            // disable all pio programs, if any are running (mclk, playback, record)
            pio_sm_set_enabled(pio1, mclkSm_, false);
            pio_sm_set_enabled(pio1, playbackSm_, false);
            pio_sm_set_enabled(pio1, recordSm_, false);
            // disable headphones, ADC boost, ADC and PGA
            setRegister(REG_PWR_MGMT_2, 0);
            // disable speaker, mixers and DAC
            setRegister(REG_PWR_MGMT_3, 0);
        }

        /** Configures the codec for simple playback from the aux line in inputs.
         
            We can't use the normal line in inputs that go through the PGA as they are also used as the headphone jack detection.    
          */
        static void playbackLineInDirect() {
            stop();
            LOG(LL_INFO, "Codec playback line in direct");
            // enable the ADC mix/boost stages (and heep the headphone drivers enabled, but muted)
            setRegister(REG_PWR_MGMT_2, RHPEN | LHPEN | RBSTEN | LBSTEN);
            // enable the RMIX and LMIX sections (keep the speaker on as well)
            setRegister(REG_PWR_MGMT_3, RSPKEN | LSPKEN | RMIXEN | LMIXEN);
            // connect the AUX inputs to the ADC boost section
            setRegister(REG_LADC_BOOST, 0b101); // 0db, use 111 for max 6db 
            setRegister(REG_RADC_BOOST, 0b101);
            // and connect the ADC boost section to the output mixers
            setRegister(REG_LMIXER, 0b10110);
            setRegister(REG_RMIXER, 0b10110);
        }

        /** Configfures the codec for aux line in playback using the digital bypass, i.e. from ADC to DAC. 
         
            This is very useful for testing and it *might* also enable the digital equalizer (?). In order to work properly, it requires MCLK and the chip to operate in master mode. 
         */
        static void playbackLineIn() {
            stop();
            LOG(LL_INFO, "Codec playback line in");
            // BCLK and FSCLK are driven by the chip for now, just to see stuff?
            // this is actually necessary for the bypass to work properly
            // TODO delete this when possible
            // setRegister(REG_CLK_CTRL_1, CLKIOEN);

            // enable the ADC and ADC mix/boost stages (and keep the headphone drivers enabled)
            setRegister(REG_PWR_MGMT_2, RHPEN | LHPEN | RBSTEN | LBSTEN | RADCEN | LADCEN);
            // enable the DAC & mixer sections, keep the speaker enabled
            setRegister(REG_PWR_MGMT_3, RSPKEN | LSPKEN | RMIXEN | LMIXEN | RDACEN | LDACEN);
            // connect the AUX inputs to the ADC boost section
            setRegister(REG_LADC_BOOST, 0b101); // 0db, use 111 for max 6db 
            setRegister(REG_RADC_BOOST, 0b101);
            // enable digital passthrough
            setRegister(REG_COMPANDING, ADDAP);
            // and connect the DAC section to the output mixers
            setRegister(REG_LMIXER, 0b10101);
            setRegister(REG_RMIXER, 0b10101);
        }

        /** Enables playback from the I2S data sent by the RP2350. 
         */
        static void playbackI2S(uint32_t sampleRate) {
            stop();
            LOG(LL_INFO, "Codec I2S playback at " << sampleRate << "Hz");
            // keep headphones enabled
            setRegister(REG_PWR_MGMT_2, RHPEN | LHPEN);
            // enable the DAC & mixer sections, keep the speaker enabled
            setRegister(REG_PWR_MGMT_3, RSPKEN | LSPKEN | RMIXEN | LMIXEN | RDACEN | LDACEN);
            // and connect the DAC section to the output mixers
            setRegister(REG_LMIXER, 0b10101);
            setRegister(REG_RMIXER, 0b10101);
            // enable master clock generation for the given sample rate
            enableMasterClock(sampleRate);
            // enable the I2S playback pio program at given BCLK (which is 34 bits per sample)
            i2s_out16_program_init(pio1, playbackSm_, playbackOffset_, RP_PIN_I2S_DAC, RP_PIN_I2S_BCLK);
            pio_sm_set_clock_speed(pio1, playbackSm_, sampleRate * 34 * 2);
            pio_sm_set_enabled(pio1, playbackSm_, true);
        }

        static void recordLineIn(uint32_t sampleRate) {
            stop();
            LOG(LL_INFO, "Codec I2S record from line in at " << sampleRate << "Hz");
            // enable the ADC and ADC mix/boost stages (), keep the hp drivers enabled
            setRegister(REG_PWR_MGMT_2, RHPEN | LHPEN | RBSTEN | LBSTEN | RADCEN | LADCEN);
            // enable the DAC & mixer sections, keep the speaker enabled, since we are recording from line in we can aplso playback it 
            setRegister(REG_PWR_MGMT_3, RSPKEN | LSPKEN | RMIXEN | LMIXEN | RDACEN | LDACEN);
            // connect the AUX inputs to the ADC boost section
            setRegister(REG_LADC_BOOST, 0b101); // 0db, use 111 for max 6db 
            setRegister(REG_RADC_BOOST, 0b101);

            // enable I2S master mode so that we output stuff even without the pio for testing
            //setRegister(REG_CLK_CTRL_1, CLKM_MCLK | CLKIOEN); 

            // enable master clock generation for the given sample rate
            enableMasterClock(sampleRate);
            // enable the I2S record pio program at given BCLK (34 bits per sample for I2S stereo 16bit sound)
            i2s_in16_program_init(pio1, recordSm_, recordOffset_, RP_PIN_I2S_ADC, RP_PIN_I2S_BCLK);
            pio_sm_set_clock_speed(pio1, recordSm_, sampleRate * 34 * 2);
            pio_sm_set_enabled(pio1, recordSm_, true);
        }

        static void recordMic(uint32_t sampleRate) {
            stop();
            LOG(LL_INFO, "Codec I2S record from microphone at " << sampleRate << "Hz");
            // enable mic bias 
            setRegister(REG_PWR_MGMT_1, MICBIASEN | REF_IMP_80K | IOBUFEN | ABIASEN);
            // enable the right channel PGA (where the mic is connected)
            // TODO this might change on the actual HW
            setRegister(REG_PWR_MGMT_2, RHPEN | LHPEN | RADCEN | RBSTEN | RPGAEN);
            // enable ALC on the right channel
            setRegister(REG_ALC_CTRL_1, ALCEN_RIGHT | ALC_MAX_GAIN_35 | ALC_MIN_GAIN_NEG_12);

            // connect the right channel ADC to the output mixers
            //setRegister(REG_LADC_BOOST, 0b101); // 0db, use 111 for max 6db 
            setRegister(REG_RADC_BOOST, 0b101010000);


            // enable master clock generation for the given sample rate
            enableMasterClock(sampleRate);
            // enable the I2S record pio program at given BCLK (34 bits per sample for I2S stereo 16bit sound)
            i2s_in16_program_init(pio1, recordSm_, recordOffset_, RP_PIN_I2S_ADC, RP_PIN_I2S_BCLK);
            pio_sm_set_clock_speed(pio1, recordSm_, sampleRate * 34 * 2);
            pio_sm_set_enabled(pio1, recordSm_, true);
        }

        static void setGPIO1(bool high) {
            // NOTE there seems to be an error in the register map and the GPIO is inverted actually. 
            setRegister(REG_GPIO, high ? GPIO1_LOW : GPIO1_HIGH);
        }

        static void resetGPIO1() {
            setRegister(REG_GPIO, GPIO1_FLOAT);
        }

        static void showRegisters() {
            LOG(LL_INFO, "Codec registers:");
            LOG(LL_INFO, "   1: " <<  hex(getRegister(1)) << " exp: 0x000d");
            LOG(LL_INFO, "   2: " <<  hex(getRegister(2)) << " exp: 0x01b0");
            LOG(LL_INFO, "   3: " <<  hex(getRegister(3)) << " exp: 0x006c"); 
            LOG(LL_INFO, "   8: " <<  hex(getRegister(8)) << " exp: 0x00??"); 
            LOG(LL_INFO, "   9: " <<  hex(getRegister(9)) << " exp: 0x00??"); 
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

Working snapshot:

LL_INFO: radio response: 0x81
LL_INFO:   frequency:     9370 [10kHz]
LL_INFO:   rssi:          38
LL_INFO:   snr:           31
LL_INFO:   multipath:     0
LL_INFO:   antCap:        1
LL_INFO: Codec registers:
LL_INFO:    1: 0x000d exp: 0x000d
LL_INFO:    2: 0x01b0 exp: 0x01b0
LL_INFO:    3: 0x006c exp: 0x006c
LL_INFO:   43: 0x0030 exp: 0x0000
LL_INFO:   47: 0x0007 exp: 0x0005
LL_INFO:   48: 0x0007 exp: 0x0005
LL_INFO:   49: 0x01ff exp: 0x0002
LL_INFO:   50: 0x01ff exp: 0x0016
LL_INFO:   51: 0x001e exp: 0x0016
LL_INFO:   52: 0x01ff exp: 0x007f
LL_INFO:   53: 0x0040 exp: 0x007f
LL_INFO:   54: 0x003f exp: 0x007f
LL_INFO:   55: 0x003f exp: 0x007f
LL_INFO: Enabling MCLK at 24576000Hz


*/            
        }

    private:

        static constexpr uint16_t UPDATE_BIT = 256;

        static constexpr uint8_t REG_RESET = 0;

        static constexpr uint8_t REG_PWR_MGMT_1 = 1;
        static constexpr uint16_t REF_IMP_80K = 0x01;
        static constexpr uint16_t MICBIASEN = 0b10000;
        static constexpr uint16_t IOBUFEN = 4;
        static constexpr uint16_t ABIASEN = 8;

        static constexpr uint8_t REG_PWR_MGMT_2 = 2;
        static constexpr uint16_t RHPEN = 256;
        static constexpr uint16_t LHPEN = 128;
        static constexpr uint16_t RBSTEN = 32;
        static constexpr uint16_t LBSTEN = 16;
        static constexpr uint16_t RPGAEN = 8; 
        static constexpr uint16_t LPGAEN = 4;
        static constexpr uint16_t RADCEN = 2;
        static constexpr uint16_t LADCEN = 1;

        static constexpr uint8_t REG_PWR_MGMT_3 = 3;
        static constexpr uint16_t LSPKEN = 64;
        static constexpr uint16_t RSPKEN = 32;
        static constexpr uint16_t RMIXEN = 8;
        static constexpr uint16_t LMIXEN = 4;
        static constexpr uint16_t RDACEN = 2;
        static constexpr uint16_t LDACEN = 1;

        static constexpr uint8_t REG_AUDIO_INTERFACE = 4;
        static constexpr uint16_t WLEN_16 = 0b0000000;
        static constexpr uint16_t WLEN_20 = 0b0100000;
        static constexpr uint16_t WLEN_24 = 0b1000000;
        static constexpr uint16_t WLEN_32 = 0b1100000;
        static constexpr uint16_t AIFMT_RJ = 0b00000;
        static constexpr uint16_t AIFMT_LJ = 0b01000;
        static constexpr uint16_t AIFMT_I2S = 0b10000;
        static constexpr uint16_t AIFMT_PCM = 0b11000;
        
        static constexpr uint8_t REG_COMPANDING = 5;
        static constexpr uint16_t ADDAP = 1;

        static constexpr uint8_t REG_CLK_CTRL_1 = 6;
        static constexpr uint16_t CLKM_MCLK = 0;
        static constexpr uint16_t CLKM_PLL = 0b100000000;
        static constexpr uint16_t CLKIOEN = 1;

        static constexpr uint8_t REG_CLK_CTRL_2 = 7;
        static constexpr uint16_t SCLKEN = 1;
        
        static constexpr uint8_t REG_GPIO = 8;
        static constexpr uint16_t GPIO1_FLOAT = 0b000;
        static constexpr uint16_t GPIO1_HIGH = 0b110;
        static constexpr uint16_t GPIO1_LOW = 0b111;


        static constexpr uint8_t REG_JACK_DETECT_1 = 9;
        static constexpr uint16_t JCKDEN = 0b1000000;
        static constexpr uint16_t JCKDIO1 = 0;
        static constexpr uint16_t JCKDIO2 = 0b010000;
        static constexpr uint16_t JCKDIO3 = 0b100000;

        static constexpr uint8_t REG_DAC_CTRL = 10;
        static constexpr uint8_t REG_LDAC_VOLUME = 11;
        static constexpr uint8_t REG_RDAC_VOLUME = 12;

        static constexpr uint8_t REG_JACK_DETECT_2 = 13;
        static constexpr uint16_t JCKDOEN1_SPEAKER = 0b00100000;
        static constexpr uint16_t JCKDOEN1_HEADPHONES = 0b00010000;
        static constexpr uint16_t JCKDOEN0_SPEAKER = 0b0010;
        static constexpr uint16_t JCKDOEN0_HEADPHONES = 0b0001;


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
        static constexpr uint16_t ALCEN_LEFT = 0b100000000;
        static constexpr uint16_t ALCEN_RIGHT = 0b10000000;
        static constexpr uint16_t ALC_MAX_GAIN_35 = 0b111000;
        static constexpr uint16_t ALC_MIN_GAIN_NEG_12 = 0b000; 


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
        static constexpr uint8_t REG_LADC_BOOST = 47;
        static constexpr uint8_t REG_RADC_BOOST = 48;


        static constexpr uint8_t REG_OUTPUT_CONTROL = 49;
        static constexpr uint16_t TSEN  = 2;
        static constexpr uint16_t SPKBST = 0b100;

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
            i2c::enqueueAndWait(RCKID_AUDIO_CODEC_I2C_ADDRESS, cmd, sizeof(cmd));
            //::i2c::masterTransmit(RCKID_AUDIO_CODEC_I2C_ADDRESS, cmd, 2, nullptr, 0);
        }

        static uint16_t getRegister(uint8_t reg) {
            uint8_t result[2];
            reg <<= 1;
            i2c::enqueueAndWait(RCKID_AUDIO_CODEC_I2C_ADDRESS, & reg, 1, result, 2);
            //::i2c::masterTransmit(RCKID_AUDIO_CODEC_I2C_ADDRESS, & reg, 1, result, 2);
            return (result[0] << 8) | result[1];;
        }

        static inline int mclkSm_ = -1;
        static inline uint mclkOffset_ = 0;

        static inline int playbackSm_ = -1;
        static inline uint playbackOffset_ = 0;

        static inline int recordSm_ = -1;
        static inline uint recordOffset_ = 0;

    }; // rckid::Codec

} // namespace rckid