#pragma once

#include "rckid.h"

namespace rckid {

    /** FM Radio Driver
     
        The driver abstracts the functionality of the Si4705 FM radio chip that is part of the mkIII device. 

        ## Communication details

        The chip is connected to the I2C bus, but the I2C communication is somewhat special as the command response times take time to process. This can be dealt with via either blocking reads of status byte until command is processed, followed by the read of the response, or by use the interrupt pin. Howwver, as per the programming guide, the command execution time is quite straightforward, being 100ms for power up and 300us for all other commands (table 50 at page 248). Additionally, there is the variable length seek complete, which is only used for tuning commands and the 10ms completion time for set property command. 

     */
    class Radio {
    public:
        /** Returns instance of the radio driver. 
         
            If the radio is not present, returns nullptr and this *must* be checked before operating the radio. 
         */
        static Radio * instance() { return instance_; }

        PACKED(class Response {
        public:
            /** When high, new command can be sent. */
            bool cts() const { return raw_ & 0x80; }
            /** When high, there was an error with the last command. */
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
            friend class Radio;

            void clearCts() { raw_ &= ~0x80; }
            void clearStc() { raw_ &= ~0x01; }

            volatile uint8_t raw_ = 0xff;
        }); // Radio::Response

        /** Version information returned by the GET_REV command. 
         */
        PACKED(class VersionInfo {
        public:
            uint8_t partNumber;
            uint8_t fwMajor;
            uint8_t fwMinor;
            uint16_t patch;
            uint8_t compMajor;
            uint8_t compMinor;
            uint8_t chipRevision;
            uint8_t reserved[5];
            uint8_t cid;
        }); // Radio::VersionInfo


        PACKED(class TuneStatus {
        public:
            bool seekBandLimit() const { return result_ & 0x80; }
            bool afcRail() const { return result_ & 0x02; }
            bool valid() const { return result_ & 0x01; }

            uint16_t frequency10kHz() const { return frequency_; }
            uint8_t rssi() const { return rssi_; }
            uint8_t snr() const { return snr_; }
            uint8_t multipath() const { return mult_; }
            uint8_t antCap() const { return antCap_; }

        private:

            friend class Radio;

            uint8_t result_;
            uint16_t frequency_;
            uint8_t rssi_;
            uint8_t snr_;
            uint8_t mult_;
            uint8_t antCap_;
        }); // Radio::TuneStatus

        PACKED(class RSQStatus {
        public:
            // TODO the interrupt
            bool valid() const { return resp2_ & 0x01; }
            bool afcRail() const { return resp2_ & 0x02; }
            bool softMute() const { return resp2_ & 0x08; }
            bool stereoPilot() const { return resp3_ & 0x80; }
            uint8_t stereo() const { return resp3_ & 0x7f; }
            uint8_t rssi() const { return rssi_; }
            uint8_t snr() const { return snr_; }
            uint8_t multipath() const { return mult_; }
            int8_t frequencyOffset() const { return freqOffset_; }

        private:
            friend class Radio;

            uint8_t resp1_;
            uint8_t resp2_;
            uint8_t resp3_;
            uint8_t rssi_;
            uint8_t snr_;
            uint8_t mult_;
            int8_t freqOffset_;

        }); // Radio::RSQStatus

        bool enabled() const { return busy_ & RADIO_ENABLED; }

        bool busy() const { return status_.cts() == false; }

        bool tuning() const { return status_.stcInt() == false; }

        /** Enables, or disables the radio chip. 
         */
        void enable(bool value);

        /** Returns the device version & software revision information.
         */
        VersionInfo getVersionInfo() {
            sendCommand({ CMD_GET_REV });
            getResponse(sizeof(VersionInfo) + 1);
            status_.versionInfo.patch = platform::swapBytes(status_.versionInfo.patch);
            return status_.versionInfo;
        }

        /** Sets the FM frequency in multiples of 10kHz, i.e. 9370 for 93.7MHz. 
            
            The frequency must be in the range of 6800 to 10800, i.e. 68MHz to 108MHz. As with any other commands, if the radio is busy, the app will block until the previous command has been processed and the radio is ready for the next command.
         */
        void setFrequency(uint16_t freq_10kHz) {
            busy_ |= RADIO_TUNE_BUSY;
            sendCommand({
                CMD_FM_TUNE_FREQ,
                0, // no freeze or fast mode
                platform::highByte(freq_10kHz),
                platform::lowByte(freq_10kHz),
                0, // automatic antenna tuning capacitor value
            });
            getResponse();
        }

        /** Starts automatic seek to the next valid channel (frequency up), with wrap around at the end of the band.
         */
        void seekUp() {
            busy_ |= RADIO_TUNE_BUSY;
            sendCommand({
                CMD_FM_SEEK_START,
                0x08 | 0x04, // seek up, wrap around
            });
            getResponse();
        }

        /** Starts automatic seek to the previous valid channel (frequency down), with wrap around at the beginning of the band.
         */
        void seekDown() {
            busy_ |= RADIO_TUNE_BUSY;
            sendCommand({
                CMD_FM_SEEK_START,
                0x04, // seek down, wrap around
            });
            getResponse();
        }

        /** Returns the current tuning status. 
         */
        TuneStatus getTuneStatus() {
            sendCommand({
                CMD_FM_TUNE_STATUS,
                0x00, // no interrupt acknowledge
            });
            getResponse(8); 
            status_.tuneStatus.frequency_ = platform::swapBytes(status_.tuneStatus.frequency_);
            return status_.tuneStatus;
        }

        RSQStatus getRSQStatus() {
            sendCommand({
                CMD_FM_RSQ_STATUS,
                0x00, // no interrupt acknowledge
            });
            getResponse(8); 
            return status_.rsqStatus;
        }

        void enableEmbeddedAntenna(bool value = true) {
            setProperty(PROP_FM_ANTENNA_INPUT, value ? 0x0001 : 0x0000);
        }

        bool embeddedAntennaEnabled() {
            return getProperty(PROP_FM_ANTENNA_INPUT) != 0;
        }

        void enableStereo(bool value = true) {
            setProperty(PROP_FM_BLEND_RSSI_STEREO_THRESHOLD, value ? 0x0031 : 0x007f);
            setProperty(PROP_FM_BLEND_MONO_THRESHOLD, value ? 0x0001 : 0x007f);
        }

        /** Enables or disables the GPO1 pin. When disabled, the pin is left floating, otherwise its either high or low, based on the last setGPO1 function call value (low by default).
         */
        void enableGPO1(bool value) {
            sendCommand({
                CMD_GPIO_CTL,
                value ? 0x02_u8 : 0x00_u8, // GPO1 output enabled
            });
            getResponse();
        }

        /** Determines the GPO1 pin value, if enabled. 
         */
        void setGPO1(bool value) {
            sendCommand({
                CMD_GPIO_SET,
                value ? 0x02_u8 : 0x00_u8, // GPO1 output value
            });
            getResponse();
        }

    private:
        // TODO delete this when done
        friend class FMRadio;

        friend void initialize(int argc, char const * argv[]);
        friend void irqGPIO_(uint pin, uint32_t events);


        PACKED(class PropertyValue {
        public:
            uint8_t result_; // always 0
            uint16_t value;
        }); // Radio::PropertyValue

        PACKED(class MaxResponse : public Response {
        public:
            PACKED(union {
                uint8_t data[15];
                PropertyValue propertyValue;
                TuneStatus tuneStatus;
                VersionInfo versionInfo;
                RSQStatus rsqStatus;
            });
        });

        static_assert(sizeof(Response) == 1);
        static_assert(sizeof(MaxResponse) == 16);
    
        static void initialize();

        /** Performs reset of the radio chip. 
         
            Reset has to be performed before any other command - the radio will not even show up as I2C device until the reset is applied. 
        */
        static void reset();

        void sendCommand(uint8_t const * cmd, uint8_t cmdSize, uint32_t ctsTime = 1);

        template<size_t N>
        void sendCommand(uint8_t const (&cmd)[N], uint32_t ctsTime = 1) { sendCommand(cmd, N, ctsTime); }

        void getResponse(uint8_t expectedBytes = 1);

        uint16_t getProperty(uint16_t property) {
            sendCommand({
                CMD_GET_PROPERTY,
                0, // an extra 0 byte from the datasheet
                platform::highByte(property),
                platform::lowByte(property),
            });
            // as per the datasheet, response is 4 bytes (status, 0, hi & lo)
            getResponse(4);
            status_.propertyValue.value = platform::swapBytes(status_.propertyValue.value);
            return status_.propertyValue.value;
        }

        void setProperty(uint16_t property, uint16_t value) {
            sendCommand({
                CMD_SET_PROPERTY,
                0, // an extra 0 byte from the datasheet
                platform::highByte(property),
                platform::lowByte(property),
                platform::highByte(value),
                platform::lowByte(value),
            }, 11); // 1 for the command, 10 for the completion
            getResponse();
        }

        void getStatus() {
            sendCommand({ CMD_GET_INT_STATUS });
            // as per the datasheet, response is 1 byte (status)
            getResponse(1);
        }



        static void irqHandler();

        static inline Radio * instance_ = nullptr;

        MaxResponse status_;

        uint8_t responseSize_ = 0;



        /** Flags to determine the radio status. 
         */
        volatile uint8_t busy_ = 0;
        static constexpr uint8_t RADIO_TUNE_BUSY = 0x02;
        static constexpr uint8_t RADIO_ENABLED = 0x80;

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
    }; // rckid::Radio

} // namespace rckid