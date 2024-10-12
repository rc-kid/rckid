class RCKid;

namespace rckid {

    /** AVR Status
     
        The AVR status is what the RP chip requests every tick and so its size must be kept to a minimum. 
    */
    PACKED(class Status {
    public:

        bool btnHome() const { return status_ & BTN_HOME; }
        bool btnVolumeUp() const { return status_ & BTN_VOL_UP; }
        bool btnVolumeDown() const { return status_ & BTN_VOL_DOWN; }
        bool charging() const { return status_ & CHARGING; }
        bool powerDC() const { return status_ & DC_POWER; }
        bool audioEnabled() const { return status_ & AUDIO_EN; }
        bool audioHeadphones() const { return status_ & AUDIO_HEADPHONES; }

        void setBtnHome(bool value) { value ? status_ |= BTN_HOME : status_ &= ~BTN_HOME; }
        void setBtnVolumeUp(bool value) { value ? status_ |= BTN_VOL_UP : status_ &= ~BTN_VOL_UP; }
        void setBtnVolumeDown(bool value) { value ? status_ |= BTN_VOL_DOWN : status_ &= ~BTN_VOL_DOWN; }
        void setCharging(bool value) { value ? status_ |= CHARGING : status_ &= ~CHARGING; }
        void setPowerDC(bool value) { value ? status_ |= DC_POWER : (status_ &= ~DC_POWER); }
        void setAudioEnabled(bool value) { value ? status_ |= AUDIO_EN : status_ &= ~AUDIO_EN; }
        void setAudioHeadphones(bool value) { value ? status_ |= AUDIO_HEADPHONES : status_ &= ~AUDIO_HEADPHONES; }
        void setVolumeKeys(bool volUp, bool volDown) {
            status_ &= ~(BTN_VOL_UP | BTN_VOL_DOWN);
            status_ |= ( volUp ? BTN_VOL_UP : 0) | (volDown ? BTN_VOL_DOWN : 0);
        }

        uint16_t vBatt() const { return voltageFromRawStorage(vBatt_); }
        int16_t temp() const { return -200 + (temp_ * 5); }

        void setVBatt(uint16_t vx100) { vBatt_ = voltageToRawStorage(vx100); }
        void setTemp(int32_t tempx10) {
            if (tempx10 <= -200)
                temp_ = 0;
            else if (tempx10 >= 1080)
                temp_ = 255;
            else 
                temp_ = (tempx10 + 200) / 5;
        }

        bool btnLeft() const { return controls_ & BTN_LEFT; }
        bool btnRight() const { return controls_ & BTN_RIGHT; }
        bool btnUp() const { return controls_ & BTN_UP; }
        bool btnDown() const { return controls_ & BTN_DOWN; }
        bool btnA() const { return controls_ & BTN_A; }
        bool btnB() const { return controls_ & BTN_B; }
        bool btnSel() const { return controls_ & BTN_SELECT; }
        bool btnStart() const { return controls_ & BTN_START; }

        void setDPadKeys(bool l, bool r, bool u, bool d) {
            controls_ &= ~(BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN);
            controls_ |= (l ? BTN_LEFT : 0) | ( r ? BTN_RIGHT : 0) | (u ? BTN_UP : 0) | (d ? BTN_DOWN : 0);
        }

        void setABSelStartKeys(bool a, bool b, bool sel, bool start) {
            controls_ &= ~(BTN_A | BTN_B | BTN_SELECT | BTN_START);
            controls_ |= (a ? BTN_A : 0) | ( b ? BTN_B : 0) | (sel ? BTN_SELECT : 0) | (start ? BTN_START : 0);
        }

    private:

        friend class ::RCKid;

        static constexpr uint8_t BTN_HOME = 1 << 0;
        static constexpr uint8_t BTN_VOL_UP = 1 << 1;
        static constexpr uint8_t BTN_VOL_DOWN = 1 << 2;
        static constexpr uint8_t CHARGING = 1 << 3;
        static constexpr uint8_t DC_POWER = 1 << 4;
        static constexpr uint8_t AUDIO_EN = 1 << 5;
        static constexpr uint8_t AUDIO_HEADPHONES = 1 << 6;
        // 7 free

        /** Device status (8 single bit values)
         */
        uint8_t status_ = 0;

        /** Battery level.
         */
        uint8_t vBatt_ = 0;

        /** Temperature measured by AVR.
         */
        uint8_t temp_ = 0;

        static constexpr uint8_t BTN_LEFT = 1 << 0;
        static constexpr uint8_t BTN_RIGHT = 1 << 1;
        static constexpr uint8_t BTN_UP = 1 << 2;
        static constexpr uint8_t BTN_DOWN = 1 << 3;
        static constexpr uint8_t BTN_A = 1 << 4;
        static constexpr uint8_t BTN_B = 1 << 5;
        static constexpr uint8_t BTN_SELECT = 1 << 6;
        static constexpr uint8_t BTN_START = 1 << 7;

        /** TODO in V3 top plate buttons are handled by the RP2350 instead. 
         */
        uint8_t controls_ = 0;

        static uint16_t voltageFromRawStorage(uint8_t value) {
            return (value == 0) ? 0 : (value + 245);
        }

        static uint8_t voltageToRawStorage(uint16_t vx100) {
            if (vx100 < 250)
                return 0;
            else if (vx100 >= 500)
                return 255;
            else 
                return (vx100 - 245) & 0xff;
        }
    }); 

    /** Extra flags and information that can be transmitted from AVR to RP as part of the transferrable state, but not the status. Mostly useful for debugging purposes. 
     */
    PACKED(class Extras {
    public:
        bool debugMode() const { return raw_ & DEBUG_MODE; }
        bool userNotification() const { return raw_ & USER_NOTIFICATION; }
        bool chargingError() const { return raw_ & CHARGING_ERROR; }

        void setDebugMode(bool value) { value ? raw_ |= DEBUG_MODE : raw_ &= ~DEBUG_MODE; }
        void setUserNotification(bool value) { value ? raw_ |= USER_NOTIFICATION : raw_ &= ~USER_NOTIFICATION; }
        void setChargingError(bool value) { value ? raw_ |= CHARGING_ERROR : raw_ &= ~CHARGING_ERROR; }

    private:
        static constexpr uint8_t DEBUG_MODE = 1 << 0;
        static constexpr uint8_t USER_NOTIFICATION = 1 << 1;
        static constexpr uint8_t CHARGING_ERROR = 1 << 2;
        uint8_t raw_ = 0;
    });

    /** The entire transferrable state. 
     
        This is the maximum extent of data that can be read from the AVR. The I2C handler is very simple and only sends the transferrable state during a master read transaction. Therefore the order of the fields of the transferrable state corresponds to the expected frequency at which they will be read. 
    */
    PACKED(class TransferrableState {
    public:
        /** The status, requested every tick. 
         */
        Status status;
        /** Current date & time as kept by the RTC. 
         */
        TinyDate time;
        /** Current brightness settings. 
         */
        uint8_t brightness;
        /** Current VCC (*100, i.e. 420 for 4.2 V)
         */
        uint16_t vcc;
        /** AVR uptime in seconds. 
         */
        uint32_t uptime;
        /** Extra information, mostly for debugging. 
         */
        Extras extras;
        /** Communications buffer. This is where commands are stored and where extra commands store the data they wish to transfer to the RP. The size is enough for a single byte command and 32 byte payload. 
         */
        uint8_t buffer[33];
    });

}
