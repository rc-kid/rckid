#pragma once

#include "platform.h"

/** TLV320AIC3204 Audio Codec with I2S DAC/ADC support. 
 
    TODO Reading the reference guide, it looks like the sleep mode current with 3V3 single supply voltage is 50mA and standby current is 180mA. This is way too high - see if (a) this is true, (b) can be optimized somehow, or perhaps even if the codec can be powered "off" by holding the reset pin low for extended periods. 

    The reference guide seems like a good start to implement the driver. 

 */
class TLV320 : public i2c::Device {
public:

    enum class Headset : uint8_t {
        None = 0x00, 
        Stereo = 0x01, 
        Cellular = 0x03,
    };


    /** Powers the device on, enables the basic blocks necessaru for communication and enters standby mode.
     
        Assumes reset hs been applied to the device before and that I2C communication is working.  
     */
    void standby() {
        // wakes up from sleep mode, this is from the sleep modes datasheet
        // step 1: configure AVDD supply
        w(0x00, 0x02, 0x09); // power up ALDO
        w(0x00, 0x01, 0x08); // disable weak AVDD to DVDD connection
        w(0x00, 0x02, 0x01); // keep ALDO powered, enable master analog power control
        // step 2: power up clock generation tree
        w(0x00, 0x05, 0x91); // power up PLL
        w(0x00, 0x12, 0x02); // keep NADC = 2, powered down for sync mode
        w(0x00, 0x13, 0x88); // power up MADC = 8
        w(0x00, 0x0c, 0x88); // power up MDAC = 8
        // step 5: configure output amplifier routing
        w(0x01, 0x0c, 0x08); // reconnect LDAC_P to HPL
        w(0x01, 0x0d, 0x08); // reconnect RDAC_P to HPR
        w(0x01, 0x0e, 0x08); // reconnect LDAC_P to LOL
        w(0x01, 0x0f, 0x08); // reconnect RDAC_P to LOR
        // step 6: power up ADCs
        w(0x00, 0x51, 0xc0); // power up LADC/RADC
        // step 7: power up DACs
        w(0x00, 0x3f, 0xd4); // power up LDAC/RDAC
        // step 8: power up NDAC
        w(0x00, 0x0b, 0x82); // power up NDAC=2, sync mode
        // step 9: power up internal amplifiers
        w(0x01, 0x10, 0x00); // HPL = 0db, unmuted
        w(0x01, 0x11, 0x00); // HPR = 0db, unmuted
        w(0x01, 0x12, 0x00); // LOL = 0db, unmuted
        w(0x01, 0x13, 0x00); // LOR = 0db, unmuted
        w(0x01, 0x09, 0x3c); // power HPs/LOs
        // step 10 wait for ADCs, DACs and amplifiers to power up
        // w 30 00 00 # Switch to Page 0 
        // # f 30 24 11xx11xx # Wait for p0_r36_b7-6/3-2 to set 
        // # f 30 25 1xxx1xxx # Wait for p0_r37_b7/3 to set 
        // # f 30 26 xxx1xxx1 # Wait for p0_r38_b4/0 to set 
        // # f 30 25 x11xx11x # Wait for p0_r37_b6-5/2-1 to set 




        // enable headset detection
        w(0x01, 0x02, 0x09); // AVDD LDO on, Analog block on (required for the headphone detection)
        w(0x00, 0x67, 0x80); // enable headset detection, headset debounce 16ms, headeset button debounce off (default values)
    }
    
    /** Enters sleep mode.
     */
    void sleep() {

    }

    /** Returns the headset type detected. 
     */
    Headset connectedHeadset() {
        return static_cast<Headset>((r(0x00, 0x67) >> 5) & 3);
    }

    /** Returns true if the headset button is currently pressed.
     */
    bool headsetButtonDown() {
        // interrupt flag 2 - current value of the button press 
        return r(0x00, 0x46) & 0x20);
    }

    /** Sets master output volume.
     */
    void setVolume(uint8_t volume) {

    }

    /** Enables the headphones output. 
     */
    void enableHeadphones(bool enable = true) {

    }

    /** Enables the line out output. 
     */
    void enableLineOut(bool enable = true) {

    }

    /** Enables playback from analog inputs.
     
        This simply routes the analog inputs to the output lines. 

        TODO playback on analog 1 input can be done directly via bypassing the ADC & DAC (MAL, MAR) and connected to the headphone drivers. So for rckid, this should be the radio output. Technically this can be tested on the devboard, by desoldering the connections and routing. 

     */
    void playbackAnalog1() {

    }
    void playbackAnalog2() {

    }
    void playbackAnalog3() {

    }

    void playbackDigital() {

    }

    /** Starts recording (ADC) on given analog input. 
     */
    void recordAnalog1() {

    }
    void recordAnalog2() {

    }
    void recordAnalog3() {

    }

    /** Ensures the MF5/GPIO pin is configured as GPIO output and outputs the desired value. 
     */
    void gpioSet(bool value = true) {
        // set the MF5/GPIO pin to output mode and set the desired value (reference guide, page 108)
        w(0x00, 0x34, 0b00001100 | value);
    }

protected:

    /** Enables the PLL To oversample using the BCLK pin. 

        The clock requirements with voltages above 1.65V are that CODEC_CLKIN must be no more than 100Mhz and PLL_CLK must be between 80Mhz and 118Mhz. Using 48kHz 16bit stereo audio, we get BCLK frequency of 1.536MHz. 

        Furthermore, the PLL_CLKIN/P must be at least 512kHz and no more than 10MHz, which the 1.536Mhz satisfies nicely. This holds also for 44.1kHz playback (1.4112MHz BCLK). The PLL_CLK is calculated as folows (reference guide, page 77):

        PLL_CLK = (PLL_CLKIN * R * J.D) / P

        So for R == 3, J == 20, D == 0 and P == 1 we get PLL_CLK frequency of 92.16MHz.
     */
    void enablePLL() {
        w(0x00, 0x05, 0b00000000); // disable PLL   
        w(0x00, 0x04, 0b00000111); // set BCLK as PLL input, set PLL as CODEC_CLKIN
        w(0x00, 0x06, 20); // J values
        w(0x00, 0x07, 0); // D values MSB
        w(0x00, 0x08, 0); // D values LSB
        w(0x00, 0x05, 0b10010011); // enable PLL, P = 1, R = 3
    }

    void w(uint8_t page, uint8_t reg, uint8_t value) {
        if (page_ != page) {
            w(page_, 0, page);
            page_ = page;
        }
        uint8_t cmd[] = { reg, value };
        i2c::masterWrite(address_, cmd, sizeof(cmd));
    }

    uint8_t r(uint8_t page, uint8_t reg) {
        if (page_ != page) {
            w(page_, 0, page);
            page_ = page;
        }
        uint8_t value;
        i2c::masterRead(address_, &reg, 1, &value, 1);
        return value;
    }

    // we remember the last set page to avoid unnecessay page switching 
    uint8_t page_ = 0xff;

}; // TLV320