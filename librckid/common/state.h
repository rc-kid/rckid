#pragma once

#include "tinytime.h"

namespace rckid {


    enum class Btn {
        Left = 1 << 0, 
        Right = 1 << 1,
        Up = 1 << 2, 
        Down = 1 << 3, 
        A = 1 << 4, 
        B = 1 << 5, 
        Select = 1 << 6, 
        Start = 1 << 7,
        Home = 1 << 8, 
        VolumeUp = 1 << 9, 
        VolumeDown = 1 << 10,
    }; // rckid::Btn

    /** RCKid's status. 

        Contains the current state of all buttons as well as the audio and power events, all monitored by the AVR. 
    */
    class Status {
    public:

        bool down(Btn b) const {
            unsigned x = static_cast<unsigned>(b);
            if (x > 0xff) {
                x >>= 8;
                return raw_[1] & x;
            } else {
                return raw_[0] & x;
            }
        }

        bool charging() const { return raw_[1] & CHARGING; }
        bool dcPower() const { return raw_[1] & DC_POWER; }
        bool headphones() const { return raw_[1] & HEADPHONES; }
        bool audioEnabled() const { return raw_[1] & AUDIO_ENABLED; }

        /** \name Setters 
         */
        //@{
        void setBtnHome(bool value = true) { value ? raw_[1] |= BTN_HOME : raw_[1] &= ~BTN_HOME; }

        void setCharging(bool value = true) { value ? raw_[1] |= CHARGING : raw_[1] &= ~CHARGING; }
        void setDCPower(bool value = true) { value ? raw_[1] |= DC_POWER : raw_[1] &= ~DC_POWER; }
        void setHeadphones(bool value = true) { value ? raw_[1] |= HEADPHONES : raw_[1] &= ~HEADPHONES; }
        void setAudioEnabled(bool value = true) { value ? raw_[1] |= AUDIO_ENABLED : raw_[1] &= ~AUDIO_ENABLED; }

        void setDPadKeys(bool l, bool r, bool u, bool d) {
            raw_[0] &= ~(DPAD_LEFT | DPAD_RIGHT | DPAD_UP | DPAD_DOWN);
            raw_[0] |= (l ? DPAD_LEFT : 0) | ( r ? DPAD_RIGHT : 0) | (u ? DPAD_UP : 0) | (d ? DPAD_DOWN : 0);
        }

        void setABXYKeys(bool a, bool b, bool sel, bool start) {
            raw_[0] &= ~(BTN_A | BTN_B | BTN_SEL | BTN_START);
            raw_[0] |= (a ? BTN_A : 0) | ( b ? BTN_B : 0) | (sel ? BTN_SEL : 0) | (start ? BTN_START : 0);
        }

        void setVolumeKeys(bool volUp, bool volDown) {
            raw_[1] &= ~(BTN_VOL_UP | BTN_VOL_DOWN);
            raw_[1] |= ( volUp ? BTN_VOL_UP : 0) | (volDown ? BTN_VOL_DOWN : 0);
        }
        //@}

    private:
        // first byte 
        static constexpr uint8_t DPAD_LEFT = static_cast<unsigned>(Btn::Left);
        static constexpr uint8_t DPAD_RIGHT = static_cast<unsigned>(Btn::Right);
        static constexpr uint8_t DPAD_UP = static_cast<unsigned>(Btn::Up);
        static constexpr uint8_t DPAD_DOWN = static_cast<unsigned>(Btn::Down);
        static constexpr uint8_t BTN_A = static_cast<unsigned>(Btn::A);
        static constexpr uint8_t BTN_B = static_cast<unsigned>(Btn::B);
        static constexpr uint8_t BTN_SEL = static_cast<unsigned>(Btn::Select);
        static constexpr uint8_t BTN_START = static_cast<unsigned>(Btn::Start);
        // second byte
        static constexpr uint8_t BTN_HOME = static_cast<unsigned>(Btn::Home) >> 8;
        static constexpr uint8_t BTN_VOL_UP = static_cast<unsigned>(Btn::VolumeUp) >> 8;
        static constexpr uint8_t BTN_VOL_DOWN = static_cast<unsigned>(Btn::VolumeDown) >> 8;
        static constexpr uint8_t CHARGING = 1 << 3;
        static constexpr uint8_t DC_POWER = 1 << 4;
        static constexpr uint8_t HEADPHONES = 1 << 5;
        static constexpr uint8_t AUDIO_ENABLED = 1 << 6;
        // 7

        // we should use array of bytes to avoid endianess mess
        uint8_t raw_[2] = {0, 0};
    } __attribute__((packed)); // rckid::Status

    /** Extra information gathered by the AVR. 
     */
    class Info {
    public:

        bool debugMode() const { return debugMode_; }

        void setDebugMode(bool value = true) { debugMode_ = value; }

        /** \name VCC
         
            The voltage to the AVR measured in 0.01[V]. Value of 0 means any voltage below 2.46V, value of 500 5V or more, otherwise the number returnes is volatge * 100. 
         */
        //@{
        uint16_t vcc() const { return (vcc_ == 0) ? 0 : (vcc_ + 245); }

        void setVcc(uint16_t vx100) {
            if (vx100 < 250)
                vcc_ = 0;
            else if (vx100 >= 500)
                vcc_ = 255;
            else 
                vcc_ = (vx100 - 245) & 0xff;
        }
        //@}

        /** \name Temperature 
         
            Returns the temperature as measured by the chip with 0.1[C] intervals. -200 is -20C or less, 1080 is 108[C] or more. 0 is 0C. 
         */
        //@{
        int16_t temp() const { return -200 + (temp_ * 5); }

        void setTemp(int32_t tempx10) {
            if (tempx10 <= -200)
                temp_ = 0;
            else if (tempx10 >= 1080)
                temp_ = 255;
            else 
                temp_ = (tempx10 + 200) / 5;
        }
        //@}

    private:
        uint8_t debugMode_;
        uint8_t vcc_;
        uint8_t temp_;
    } __attribute__((packed)); // rckid::Info

    class Config {
    public:
        uint8_t backlight() const { return backlight_; }
        void setBacklight(uint8_t value) { backlight_ = value; }

    private:
        uint8_t backlight_;

    } __attribute__((packed)); // rckid::Config

    /** LED effects 
     
        - StaticColor - display color (color, timer?)
        - Fadedisplay color, vary brightness (color, speed, min, max)
        - display color, vary hue (hue, speed)
     
     */
    class RGBEffect {
    public:
        enum class Kind : uint8_t {
            ColorStatic, 
            ColorBreathe,
            Rainbow,
        }; 

        class ColorStatic {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint16_t timeout;
        } __attribute__((packed)); 

        class ColorBreathe {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t speed;
        } __attribute__((packed)); 

        class Rainbow {
            uint8_t hueStart;
            uint8_t speed;            
        } __attribute__((packed));

        Kind kind;
        union {
            ColorStatic colorStatic;
            ColorBreathe colorBreathe;
            Rainbow rainbox;
        }  __attribute__((packed));
    } __attribute__((packed)); 

    

    /** The entire state of the device. 
        
        This is what the AVR chip will always return when asked to write data. Contains the status first, followed by all other data the AVR is in charge of or simply persisting. 
    */
    class State {
    public:

        Status status;
        Info info;
        Config config;
        TinyDate time;
        uint16_t dbg;
        // this is page EEPROM page size (32bytes) + 1 byte for command / control
        uint8_t buffer[33];

    } __attribute__((packed)); // rckid::State

    static_assert(sizeof(State) <= 256 && "I2C buffer counter on AVR is single byte");





    /** RCKid mode of operation. 
     
        The device can be in one of the following modes:

        Mode      | 3V3 | Ticks | RP2040
        ----------|-----|-------|--------|
        Normal    |  x  |   x   |    x   |
        Sleep     |  x  |   x   |        |
        PowerOff  |     |       |        |
        Wakeup    |     |   x   |        |

     */
    enum class DeviceMode : uint8_t {
        Normal, 
        Sleep, 
        PowerOff, 
        Wakeup,
    }; 



    /** Device state

        - 4 dpad buttons
        - 4 abxy buttons

        - 2 volume buttons
        - 1 headphones
        - 1 audio en

        - 1 home button
        - 1 alarm


        - 3 bits mode
        - 1 bit debug mode
        - 1 DC pwr
        - 1 charging

        - 8 bits VCC
        - 8 bits VBATT
        - 8 bits temp
        - 8 bits icharge
     */
    class State2 {
    public:

        /** \name Device Mode
         */
        //@{
        DeviceMode deviceMode() const { return static_cast<DeviceMode>(data_[2] & DEVICE_MODE); }
#ifdef RCKID_AVR
        void setDeviceMode(DeviceMode mode) {
            data_[2] &= ~DEVICE_MODE;
            data_[2] |= static_cast<uint8_t>(mode) & DEVICE_MODE;
        }
#endif
        //@}

        /** \name Buttons
         */
        //@{
        bool btnLeft() const { return data_[0] & BTN_LEFT; }
        bool btnRight() const { return data_[0] & BTN_RIGHT; }
        bool btnUp() const { return data_[0] & BTN_UP; }
        bool btnDown() const { return data_[0] & BTN_DOWN; }
        bool btnA() const { return data_[0] & BTN_A; }
        bool btnB() const { return data_[0] & BTN_B; }
        bool btnSel() const { return data_[0] & BTN_SELECT; }
        bool btnStart() const { return data_[0] & BTN_START; }
        bool btnVolUp() const { return data_[1] & BTN_VOL_UP; }
        bool btnVolDown() const { return data_[1] & BTN_VOL_DOWN; }
        bool btnHome() const { return data_[2] & BTN_HOME; }

#ifdef RCKID_AVR
        void setDPadKeys(bool l, bool r, bool u, bool d) {
            data_[0] &= ~(BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN);
            data_[0] |= (l ? BTN_LEFT : 0) | ( r ? BTN_RIGHT : 0) | (u ? BTN_UP : 0) | (d ? BTN_DOWN : 0);
        }

        void setABXYKeys(bool a, bool b, bool sel, bool start) {
            data_[0] &= ~(BTN_A | BTN_B | BTN_SELECT | BTN_START);
            data_[0] |= (a ? BTN_A : 0) | ( b ? BTN_B : 0) | (sel ? BTN_SELECT : 0) | (start ? BTN_START : 0);
        }

        void setVolumeKeys(bool volUp, bool volDown) {
            data_[1] &= ~(BTN_VOL_UP | BTN_VOL_DOWN);
            data_[1] |= ( volUp ? BTN_VOL_UP : 0) | (volDown ? BTN_VOL_DOWN : 0);
        }

        void setBtnHome(bool value = true) { value ? data_[2] |= BTN_HOME : data_[2] &= ~BTN_HOME; }
#endif
        //@}

        /** \name Current state flags
         */
        //@{
        bool headphones() const { return data_[1] & HEADPHONES; }
        bool audioEnabled() const { return data_[1] & AUDIO_EN; }
        bool debugMode() const { return data_[2] & DEBUG_MODE; }
        bool dcPower() const { return data_[2] & DC_PWR; }
        bool charging() const { return data_[2] & CHARGING; }
        bool alarm() const { return data_[2] & ALARM; }

#ifdef RCKID_AVR
        void setHeadphones(bool value) { value ? data_[1] |= HEADPHONES : data_[1] &= ~HEADPHONES; }
        void setAudioEnabled(bool value) { value ? data_[1] |= AUDIO_EN : data_[1] &= ~AUDIO_EN; }
        void setDebugMode(bool value) { value ? data_[2] |= DEBUG_MODE : data_[2] &= ~DEBUG_MODE; }
        void setDCPower(bool value) { value ? data_[2] |= DC_PWR : data_[2] &= ~DC_PWR; }
        void setCharging(bool value) { value ? data_[2] |= CHARGING : data_[2] &= ~CHARGING; }
        void setAlarm(bool value) { value ? data_[2] |= ALARM : data_[2] &= ~ALARM; }
#endif
        //@}

        /** \name VCC
         
            The voltage to the AVR measured in 0.01[V]. Value of 0 means any voltage below 2.46V, value of 500 5V or more, otherwise the number returnes is volatge * 100. 
         */
        //@{
        uint16_t vcc() const { return (data_[3] == 0) ? 0 : (data_[3] + 245); }
#ifdef RCKID_AVR
        void setVcc(uint16_t vx100) {
            if (vx100 < 250)
                data_[3] = 0;
            else if (vx100 >= 500)
                data_[3] = 255;
            else 
                data_[3] = (vx100 - 245) & 0xff;
        }
#endif

        //@}

        /** \name Battery Voltage
         */
        //@{
        uint16_t vBatt() const { return (data_[4] == 0) ? 0 : (data_[4] + 245); }

#ifdef RCKID_AVR
        void setVBatt(uint16_t vx100) {
            if (vx100 < 250)
                data_[4] = 0;
            else if (vx100 >= 500)
                data_[4] = 255;
            else 
                data_[4] = (vx100 - 245) & 0xff;
        }
#endif

        //@}

        /** \name Temperature 
         
            Returns the temperature as measured by the chip with 0.1[C] intervals. -200 is -20C or less, 1080 is 108[C] or more. 0 is 0C. 
         */
        //@{
        int16_t temp() const { return -200 + (data_[5] * 5); }

#ifdef RCKID_AVR
        void setTemp(int32_t tempx10) {
            if (tempx10 <= -200)
                data_[5] = 0;
            else if (tempx10 >= 1080)
                data_[5] = 255;
            else 
                data_[5] = (tempx10 + 200) / 5;
        }
#endif
        //@}

        /** \name Screen brightness
        */
        //@{
        uint8_t brightness() const { return data_[6]; }
#ifdef RCKID_AVR
        void setBrightness(uint8_t value) { data_[6] = value; }
#endif
        //@}

    private:

        static constexpr uint8_t BTN_LEFT = 1 << 0;
        static constexpr uint8_t BTN_RIGHT = 1 << 1;
        static constexpr uint8_t BTN_UP = 1 << 2;
        static constexpr uint8_t BTN_DOWN = 1 << 3;
        static constexpr uint8_t BTN_A = 1 << 4;
        static constexpr uint8_t BTN_B = 1 << 5;
        static constexpr uint8_t BTN_SELECT = 1 << 6;
        static constexpr uint8_t BTN_START = 1 << 7;

        static constexpr uint8_t BTN_VOL_UP = 1 << 0;
        static constexpr uint8_t BTN_VOL_DOWN = 1 << 1;
        static constexpr uint8_t HEADPHONES = 1 << 2;
        static constexpr uint8_t AUDIO_EN = 1 << 3;
        // 4
        // 5
        // 6
        // 7

        static constexpr uint8_t DEVICE_MODE = 7; // 3
        static constexpr uint8_t DEBUG_MODE = 1 << 3;
        static constexpr uint8_t DC_PWR = 1 << 4;
        static constexpr uint8_t CHARGING = 1 << 5;
        static constexpr uint8_t BTN_HOME = 1 << 6;
        static constexpr uint8_t ALARM = 1 << 7;

        uint8_t data_[8];

    } __attribute__((packed)); 

    static_assert(sizeof(State2) == 8);



    struct TransferrableState {
        State2 state;
        // TODO datetime
        uint8_t buffer[33];
    };

} // namespace rckid