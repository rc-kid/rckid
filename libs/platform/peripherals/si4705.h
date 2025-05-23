#pragma once

#include "platform.h"

/** FM Radio Chip
 
    Technically, this driver will work with all kinds of Si FM receivers, not just Si4705 (which is the fancy I2S ADC (which we are not using), RDS and 2 external antennas). Unlike most other I2C devices, the chip does not have registers, but instead uses commands and properties to accomplish control.

 */
class Si4705 : public i2c::Device {
public:

    /** Response byte. This is returned by every command. 
     */
    PACKED(class Response {
    public:
        /** When high, new command can be sent. */
        bool cts() const { return raw_ & 0x80; }
        /** True when there was an error with the last command. */
        bool err() const { return raw_ & 0x40; }
        /** Received signal quality interrupt (active high). */
        bool rsqInt() const { return raw_ & 0x08; }
        /** RDS interrupt (active high). */
        bool rdsInt() const { return raw_ & 0x04; }
        /** Seek/Tune complete interrupt (active high). */
        bool stcInt() const { return raw_ & 0x01; }

        /** Returns the raw vresponse code. */
        uint8_t raw() const { return raw_; }
    private:
        friend class Si4705;
        uint8_t raw_;
    }); // Si4705::Response

    /** Version information returned by the GET_REV command. 
     */
    PACKED(class VersionInfo {
    public:
        Response response;
        uint8_t partNumber;
        uint8_t fwMajor;
        uint8_t fwMinor;
        uint16_t patch;
        uint8_t compMajor;
        uint8_t compMinor;
        uint8_t chipRevision;
        uint8_t reserved[5];
        uint8_t cid;
    }); // Si4705::VersionInfo

    static constexpr uint8_t I2C_ADDRESS = 0x11;

    /** Performs reset of the radio chip. 
     
        Reset has to be performed before any other command - the radio will not even show up as I2C device until the reset is applied. 
     */
    void reset(gpio::Pin resetPin) {
        gpio::outputLow(resetPin);
        cpu::delayMs(10);
        gpio::setAsInputPullup(resetPin);
    }

    /** Turns the radio off. 
     
        Note that GPIOs do not work when powered off. 
     */
    Response powerOff() {
        uint8_t cmd = CMD_POWER_DOWN;
        Response response;
        i2c::masterTransmit(I2C_ADDRESS, & cmd, 1, & response.raw_, 1);
        return response;
    }

    /** Powers the radio on and starts receiving on the FM band. 
     */
    Response powerOn(bool enableIrq = true) {
        uint8_t cmd[] = { 
            CMD_POWER_UP,
            (enableIrq ? 0x80 : 0x00) | 0x10, // CTS interrupt enabled(?) + external oscillator
            0b00000101, // FM receive opmode
        };
        Response response;
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), & response.raw_, 1);
        return response;
    }

    VersionInfo getRevision() {
        uint8_t cmd = CMD_GET_REV;
        VersionInfo version;
        i2c::masterTransmit(I2C_ADDRESS, & cmd, 1, (uint8_t *) & version, sizeof(version));
        return version;
    }

    Response getStatus() {
        uint8_t cmd = CMD_GET_INT_STATUS;
        Response response;
        i2c::masterTransmit(I2C_ADDRESS, & cmd, 1, & response.raw_, 1);
        return response;
    }

    Response enableGPO1(bool value) {
        uint8_t cmd[] = { CMD_GPIO_CTL, value ? 0x02 : 0x00 };
        Response response;
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), & response.raw_, 1);
        return response;
    }

    Response setGPO1(bool value) {
        uint8_t cmd[] = { CMD_GPIO_SET, value ? 0x02 : 0x00 };
        Response response;
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), & response.raw_, 1);
        return response;
    }

private:

    /* Commands - from 5.2 of AN332, Si4705 programming. 
    */
    static constexpr uint8_t CMD_POWER_UP = 0x01;
    static constexpr uint8_t CMD_GET_REV = 0x10;
    static constexpr uint8_t CMD_POWER_DOWN = 0x11;
    static constexpr uint8_t CMD_SET_PROPERTY = 0x12;
    static constexpr uint8_t CMD_GET_PROPERTY = 0x13;
    static constexpr uint8_t CMD_GET_INT_STATUS = 0x14;
    static constexpr uint8_t CMD_PATCH_ARGS = 0x15;
    static constexpr uint8_t CMD_PATCH_DATA = 0x16;
    static constexpr uint8_t CMD_FM_TUNE_FREQ = 0x20;
    static constexpr uint8_t CMD_FM_SEEK_START = 0x21;
    static constexpr uint8_t CMD_FM_TUNE_STATUS = 0x22;
    static constexpr uint8_t CMD_FM_RSQ_STATUS = 0x23;
    static constexpr uint8_t CMD_FM_RDS_STATUS = 0x24;
    static constexpr uint8_t CMD_FM_AGC_STATUS = 0x27;
    static constexpr uint8_t CMD_FM_AGC_OVERRIDE = 0x28;
    static constexpr uint8_t CMD_GPIO_CTL = 0x80;
    static constexpr uint8_t CMD_GPIO_SET = 0x81;

    /* Properties - from 5.2 AN332, Si4705 programming. 
     */
    static constexpr uint16_t PROP_GPO_IEN = 0x0001;
    static constexpr uint16_t PROP_DIGITAL_OUTPUT_FORMAT = 0x0102;
    static constexpr uint16_t PROP_DIGITAL_OUTPUT_SAMPLE_RATE = 0x104;
    static constexpr uint16_t PROP_REFCLK_FREQ = 0x0201;
    static constexpr uint16_t PROP_REFCLK_PRESCALE = 0x0202;
    static constexpr uint16_t PROP_FM_DEEMPHASIS = 0x1100;
    static constexpr uint16_t PROP_FM_CHANNEL_FILTER = 0x1102;
    static constexpr uint16_t PROP_FM_BLEND_STEREO_THRESHOLD = 0x1105;
    static constexpr uint16_t PROP_FM_BLEND_MONO_THRESHOLD = 0x1106;
    static constexpr uint16_t PROP_FM_ANTENNA_INPUT = 0x1107;
    static constexpr uint16_t PROP_FM_MAX_TUNE_ERROR = 0x1108;
    static constexpr uint16_t PROP_FM_RSQ_INT_SOURCE = 0x1200;
    static constexpr uint16_t PROP_FM_RSQ_SNR_HI_THRESHOLD = 0x1201;
    static constexpr uint16_t PROP_FM_RSQ_SNR_LO_THRESHOLD = 0x1202;
    static constexpr uint16_t PROP_FM_RSQ_RSSI_HI_THRESHOLD = 0x1203;
    static constexpr uint16_t PROP_FM_RSQ_RSSI_LO_THRESHOLD = 0x1204;
    static constexpr uint16_t PROP_FM_RSQ_MULTIPATH_HI_THRESHOLD = 0x1205;
    static constexpr uint16_t PROP_FM_RSQ_MULTIPATH_LO_THRESHOLD = 0x1206;
    static constexpr uint16_t PROP_FM_RSQ_BLEND_THRESHOLD = 0x1207;
    static constexpr uint16_t PROP_FM_SOFT_MUTE_RATE = 0x1300;
    static constexpr uint16_t PROP_FM_SOFT_MUTE_SLOPE = 0x1301;
    static constexpr uint16_t PROP_FM_SOFT_MUTE_MAX_ATTENUATION = 0x1302;
    static constexpr uint16_t PROP_FM_SOFT_MUTE_SNR_THRESHOLD = 0x1303;
    static constexpr uint16_t PROP_FM_SOFT_MUTE_RELEASE_RATE = 0x1304;
    static constexpr uint16_t PROP_FM_SOFT_MUTE_ACCACK_RATE = 0x1305;
    static constexpr uint16_t PROP_FM_SEEK_BAND_BOTTOM = 0x1400;
    static constexpr uint16_t PROP_FM_SEEK_BAND_TOP = 0x1401;
    static constexpr uint16_t PROP_FM_SEEK_FREQ_SPACING = 0x1402;
    static constexpr uint16_t PROP_FM_SEEK_TUNE_SNR_THRESHOLD = 0x1403;
    static constexpr uint16_t PROP_FM_SEEK_TUNE_RSSI_THRESHOLD = 0x1404;
    static constexpr uint16_t PROP_FM_RDS_INT_SOURCE = 0x1500;
    static constexpr uint16_t PROP_FM_RDS_INT_FIFO_COUNT = 0x1501;
    static constexpr uint16_t PROP_FM_RDS_CONFIG = 0x1502;
    static constexpr uint16_t PROP_FM_RDS_CONFIDENCE = 0x1503;
    static constexpr uint16_t PROP_FM_AGC_ATTACK_RATE = 0x1700;
    static constexpr uint16_t PROP_FM_AGC_RELEASE_RATE = 0x1701;
    static constexpr uint16_t PROP_FM_BLEND_RSSI_STEREO_THRESHOLD = 0x1800;
    static constexpr uint16_t PROP_FM_BLEND_RSSI_MONO_THRESHOLD = 0x1801;
    static constexpr uint16_t PROP_FM_BLEND_RSSI_ATTACK_RATE = 0x1802;
    static constexpr uint16_t PROP_FM_BLEND_RSSI_RELEASE_RATE = 0x1803;
    static constexpr uint16_t PROP_FM_BLEND_SNR_STERO_THRESHOLD = 0x1804;
    static constexpr uint16_t PROP_FM_BLEND_SNR_MONO_THRESHOLD = 0x1805;
    static constexpr uint16_t PROP_FM_BLEND_SNR_ATTACK_RATE = 0x1806;
    static constexpr uint16_t PROP_FM_BLEND_SNR_RELEASE_RATE = 0x1807;
    static constexpr uint16_t PROP_FM_BLEND_MULTIPATH_STEREO_THRESHOLD = 0x1808;
    static constexpr uint16_t PROP_FM_BLEND_MULTIPATH_MONO_THRESHOLD = 0x1809;
    static constexpr uint16_t PROP_FM_BLEND_MULTIPATH_ATTACK_RATE = 0x180a;
    static constexpr uint16_t PROP_FM_BLEND_MULTIPATH_RELEASE_RATE = 0x180b;
    static constexpr uint16_t PROP_FM_BLEND_MAX_STEREO_SEPARATION = 0x180c;
    static constexpr uint16_t PROP_FM_NB_DETECT_THRESHOLD = 0x1900;
    static constexpr uint16_t PROP_FM_NB_INTERVAL = 0x1901;
    static constexpr uint16_t PROP_FM_NB_RATE = 0x1902;
    static constexpr uint16_t PROP_FM_NB_IIR_FILTER = 0x1903;
    static constexpr uint16_t PROP_FM_NB_DELAY = 0x1904;
    static constexpr uint16_t PROP_FM_HICUT_SNR_HIGH_THRESHOLD = 0x1a00;
    static constexpr uint16_t PROP_FM_HICUT_SNR_LOW_THRESHOLD = 0x1a01;
    static constexpr uint16_t PROP_FM_HICUT_ATTACK_RATE = 0x1a02;
    static constexpr uint16_t PROP_FM_HICUT_RELEASE_RATE = 0x1a03;
    static constexpr uint16_t PROP_FM_HICUT_MULTIPATH_TRIGGER_THRESHOLD = 0x1a04;
    static constexpr uint16_t PROP_FM_HICUT_MULTIPATH_END_THRESHOLD = 0x1a05;
    static constexpr uint16_t PROP_FM_HICUT_CUTOFF_FREQUENCY = 0x1a06;
    static constexpr uint16_t PROP_RX_VOLUME = 0x4000;
    static constexpr uint16_t PROP_RX_HARD_MUTE = 0x4001;

}; // Si4705