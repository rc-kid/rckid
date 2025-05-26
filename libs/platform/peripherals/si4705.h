#pragma once

#include <platform.h>
#include <platform/utils.h>

/** FM Radio Chip
 
    Technically, this driver will work with all kinds of Si FM receivers, not just Si4705 (which is the fancy I2S ADC (which we are not using), RDS and 2 external antennas). Unlike most other I2C devices, the chip does not have registers, but instead uses commands and properties to accomplish control.

    A command is sent as a separate START-STOP write transaction. Each command returns a response, which is either a single status byte, or a status byte followed by the response data. There is however a catch that the response can only be read after the CTS bit in the first response byte (the status) is set. So we need to be polling the first status byte until its MSB is set, after which we can read the rest of the response data. 

    Alternativel the CTS IRQ can be enabled, in which case the chip will send an interrupt when the CTS bit is ready and the response can be read in full.

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
        uint8_t rawResponse() const { return raw_; }
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
    private:
        friend class Si4705;
    }); // Si4705::VersionInfo

    PACKED(class TuneStatus {
    public:
        Response response;

        bool seekBandLimit() const { return result_ & 0x80; }
        bool afcRail() const { return result_ & 0x02; }
        bool valid() const { return result_ & 0x01; }

        uint16_t frequency10kHz() const { return frequency_; }
        uint8_t rssi() const { return rssi_; }
        uint8_t snr() const { return snr_; }
        uint8_t multipath() const { return mult_; }
        uint8_t antCap() const { return antCap_; }

    private:
        friend class Si4705;
        uint8_t result_;
        uint16_t frequency_;
        uint8_t rssi_;
        uint8_t snr_;
        uint8_t mult_;
        uint8_t antCap_;

    }); // Si4705::TuneStatus

    PACKED(class SignalStatus {
    public:
        Response response;

        // TODO

    }); // Si4705::SignalStatus

    Si4705() : i2c::Device{I2C_ADDRESS} {}

    static constexpr uint8_t I2C_ADDRESS = 0x11;

    /** Performs reset of the radio chip. 
     
        Reset has to be performed before any other command - the radio will not even show up as I2C device until the reset is applied. 
     */
    void reset(gpio::Pin resetPin) {
        gpio::outputLow(resetPin);
        cpu::delayMs(10);
        gpio::setAsInputPullup(resetPin);
    }

    /** Returns response of the last command sent to the radio chip.
     */
    Response lastResponse() const { return last_; }

    /** Turns the radio off. 
     
        Note that GPIOs do not work when powered off. 
     */
    Response powerOff() {
        uint8_t cmd = CMD_POWER_DOWN;
        i2c::masterTransmit(I2C_ADDRESS, & cmd, 1, & last_.raw_, 1);
        return last_;
    }

    /** Powers the radio on and starts receiving on the FM band. 
     */
    void powerOn() {
        uint8_t cmd[] = { 
            CMD_POWER_UP,
            0, //CMD_POWER_UP_CTSIEN | CMD_POWER_UP_GPO2OEN | CMD_POWER_UP_XOSCEN,
            CMD_POWER_UP_FM_RECEIVER,
        };
        i2c::masterWrite(I2C_ADDRESS, cmd, sizeof(cmd));
    }

    /** Returns the device version & software revision information.
     */
    VersionInfo getVersion() {
        uint8_t cmd = CMD_GET_REV;
        VersionInfo version;
        i2c::masterWrite(I2C_ADDRESS, cmd, 1);
        i2c::masterRead(I2C_ADDRESS, (uint8_t *) & version, sizeof(version));
        last_ = version.response;
        version.patch = platform::swapBytes(version.patch);
        return version;
    }

    /** Sets the given property ID to provided value. 
     
        NOTE this is very low level interface and should be used with care.
     */
    Response setProperty(uint16_t property, uint16_t value) {
        uint8_t cmd[] = {
            CMD_SET_PROPERTY,
            0, // an extra 0 byte from the datasheet
            platform::highByte(property),
            platform::lowByte(property),
            platform::highByte(value),
            platform::lowByte(value),
        };
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), & last_.raw_, 1);
        return last_;
    }

    /** Returns the raw value of gien property. 
     */
    uint16_t getProperty(uint16_t property) {
        uint8_t cmd[] = {
            CMD_GET_PROPERTY,
            0, // an extra 0 byte from the datasheet
            platform::highByte(property),
            platform::lowByte(property),
        };
        // the response is 4 bytes, first byte is the response code, second byte is reserved, third and fourth are the property value
        uint8_t response[4];
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), response, sizeof(response));
        last_.raw_ = response[0];
        return (static_cast<uint16_t>(response[2]) << 8) | response[3];
    }

    /** Returns device status (the response byte). 
     */
    Response getStatus() {
        uint8_t cmd = CMD_GET_INT_STATUS;
        i2c::masterWrite(I2C_ADDRESS, & cmd, 1);
        i2c::masterRead(I2C_ADDRESS, & last_.raw_, 1);
        return last_;
    }

    /** Commands the radio to tune the given frequency.
     
        Valid frequency range is from 68MHz to 108MHz with 10kHz resolution, i.e. values from 6800 to 10800 are permitted. In order words, the value is the frequency in MHz multipled by 100, so for instance 93.7Mhz is 9370.

        TODO freeze & fast & how the command stops, etc? 
     */
    Response tuneFrequency(uint16_t freq10kHz) {
        uint8_t cmd[] = {
            CMD_FM_TUNE_FREQ,
            0, // no freeze or fast mode
            platform::highByte(freq10kHz),
            platform::lowByte(freq10kHz),
            0, // automatic antenna tuning capacitor value
        };
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), & last_.raw_, 1);
        return last_;
    }

    /** Starts automatic seek to the next valid channel (frequency up), with wrap around at the end of the band.
     */
    Response seekUp() {
        uint8_t cmd[] = {
            CMD_FM_SEEK_START,
            0x08 | 0x04, // seek up, wrap around
        };
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), & last_.raw_, 1);
        return last_;
    }

    /** Starts automatic seek to the previous valid channel (frequency down), with wrap around at the beginning of the band.
     */
    Response seekDown() {
        uint8_t cmd[] = {
            CMD_FM_SEEK_START,
            0x04, // seek down, wrap around
        };
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), & last_.raw_, 1);
        return last_;
    }

    /** Returns the current tune status and optionally clears the Seek/Tune Complete interrput (SIC). 
     */
    TuneStatus getTuneStatus(bool intAck = false) {
        uint8_t cmd[] = {
            CMD_FM_TUNE_STATUS,
            intAck ? 0x01 : 0x00, // acknowledge the interrupt if requested
        };
        TuneStatus result;
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd),  (uint8_t *) & result, sizeof(result));
        result.frequency_ = platform::swapBytes(result.frequency_);
        last_ = result.response;
        return result;
    }

    /** Enables or disables the GPO1 pin. When disabled, the pin is left floating, otherwise its either high or low, based on the last setGPO1 function call value (low by default).
     */
    Response enableGPO1(bool value) {
        uint8_t cmd[] = { CMD_GPIO_CTL, value ? 0x02 : 0x00 };
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), & last_.raw_, 1);
        return last_;
    }

    /** Determines the GPO1 pin value, if enabled. 
     */
    Response setGPO1(bool value) {
        uint8_t cmd[] = { CMD_GPIO_SET, value ? 0x02 : 0x00 };
        i2c::masterTransmit(I2C_ADDRESS, cmd, sizeof(cmd), & last_.raw_, 1);
        return last_;
    }

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

private:

    /* Commands - from 5.2 of AN332, Si4705 programming. 
    */
    static constexpr uint8_t CMD_POWER_UP = 0x01;
    static constexpr uint8_t CMD_POWER_UP_CTSIEN = 0x80; // CTS interrupt enabled
    static constexpr uint8_t CMD_POWER_UP_GPO2OEN = 0x40; // GPO2 output enabled (interrupt)
    static constexpr uint8_t CMD_POWER_UP_PATCH = 0x20; // patch mode enabled (not used)
    static constexpr uint8_t CMD_POWER_UP_XOSCEN = 0x10; // external oscillator enabled
    static constexpr uint8_t CMD_POWER_UP_FM_RECEIVER = 0b00000101; // FM receive mode
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

    

    Response last_; 

}; // Si4705