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
        // wakes up from sleep mode, enables headphone detection



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
        if (enable) 

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

protected:

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