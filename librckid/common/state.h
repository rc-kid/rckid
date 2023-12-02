#pragma once

#include "tinytime.h"

namespace rckid {

    /** RCKid's status. 

        Contains the current state of all buttons as well as the audio and power events, all monitored by the AVR. 
    */
    class Status {
    public:
        bool dpadLeft() const { return raw_[0] & DPAD_LEFT; }
        bool dpadRight() const { return raw_[0] & DPAD_RIGHT; }
        bool dpadUp() const { return raw_[0] & DPAD_UP; }
        bool dpadDown() const { return raw_[0] & DPAD_DOWN; }
        bool btnA() const { return raw_[0] & BTN_A; }
        bool btnB() const { return raw_[0] & BTN_B; }
        bool btnSelect() const { return raw_[0] & BTN_SEL; }
        bool btnStart() const { return raw_[0] & BTN_START; }

        bool btnHome() const { return raw_[1] & BTN_HOME; }
        bool btnVolUp() const { return raw_[1] & BTN_VOL_UP; }
        bool btnVolDown() const { return raw_[1] & BTN_VOL_DOWN; }
        bool charging() const { return raw_[1] & CHARGING; }
        bool dcPower() const { return raw_[1] & DC_POWER; }
        bool headphones() const { return raw_[1] & HEADPHONES; }
        bool audioEnabled() const { return raw_[1] & AUDIO_ENABLED; }

#if (defined ARCH_AVR_MEGATINY)

        void setDpadLeft(bool value = true) { value ? raw_[0] |= DPAD_LEFT : raw_[0] &= ~DPAD_LEFT; }
        void setDpadRight(bool value = true) { value ? raw_[0] |= DPAD_RIGHT : raw_[0] &= ~DPAD_RIGHT; }
        void setDpadUp(bool value = true) { value ? raw_[0] |= DPAD_UP : raw_[0] &= ~DPAD_UP; }
        void setDpadDown(bool value = true) { value ? raw_[0] |= DPAD_DOWN : raw_[0] &= ~DPAD_DOWN; }
        void setBtnA(bool value = true) { value ? raw_[0] |= BTN_A : raw_[0] &= ~BTN_A; }
        void setBtnB(bool value = true) { value ? raw_[0] |= BTN_B : raw_[0] &= ~BTN_B; }
        void setBtnSelect(bool value = true) { value ? raw_[0] |= BTN_SEL : raw_[0] &= ~BTN_SEL; }
        void setBtnStart(bool value = true) { value ? raw_[0] |= BTN_START : raw_[0] &= ~BTN_START; }

        void setBtnHome(bool value = true) { value ? raw_[1] |= BTN_HOME : raw_[1] &= ~BTN_HOME; }
        void setBtnVolUp(bool value = true) { value ? raw_[1] |= BTN_VOL_UP : raw_[1] &= ~BTN_VOL_UP; }
        void setBtnVolDown(bool value = true) { value ? raw_[1] |= BTN_VOL_DOWN : raw_[1] &= ~BTN_VOL_DOWN; }
        void setCharging(bool value = true) { value ? raw_[1] |= CHARGING : raw_[1] &= ~CHARGING; }
        void setDCPower(bool value = true) { value ? raw_[1] |= DC_POWER : raw_[1] &= ~DC_POWER; }
        void setHeadphones(bool value = true) { value ? raw_[1] |= HEADPHONES : raw_[1] &= ~HEADPHONES; }
        void setAudioEnabled(bool value = true) { value ? raw_[1] |= AUDIO_ENABLED : raw_[1] &= ~AUDIO_ENABLED; }

        static uint16_t calculateDPadValue(bool l, bool r, bool u, bool d) {
            return (l ? DPAD_LEFT : 0) | ( r ? DPAD_RIGHT : 0) | (u ? DPAD_UP : 0) | (d ? DPAD_DOWN : 0);
        }

        void setDPadValue(uint8_t value) {
            raw_[0] &= ~(DPAD_LEFT | DPAD_RIGHT | DPAD_UP | DPAD_DOWN);
            raw_[0] |= value;
        }

        static uint8_t calculateABXYValue(bool a, bool b, bool sel, bool start) {
            return (a ? BTN_A : 0) | ( b ? BTN_B : 0) | (sel ? BTN_SEL : 0) | (start ? BTN_START : 0);
        }

        void setABXYValue(uint8_t value) {
            raw_[0] &= ~(BTN_A | BTN_B | BTN_SEL | BTN_START);
            raw_[0] |= value;
        }

        static uint8_t calculateControlValue(bool home, bool up, bool down) {
            return (home ? BTN_HOME : 0) | ( up ? BTN_VOL_UP : 0) | (down ? BTN_VOL_DOWN : 0);
        }

        void setControlValue(uint8_t value) {
            raw_[1] &= ~(BTN_HOME | BTN_VOL_UP | BTN_VOL_DOWN);
            raw_[1] |= value;
        }
#endif

    private:
        // first byte 
        static constexpr uint8_t DPAD_LEFT = 1 << 0;
        static constexpr uint8_t DPAD_RIGHT = 1 << 1;
        static constexpr uint8_t DPAD_UP = 1 << 2;
        static constexpr uint8_t DPAD_DOWN = 1 << 3;
        static constexpr uint8_t BTN_A = 1 << 4;
        static constexpr uint8_t BTN_B = 1 << 5;
        static constexpr uint8_t BTN_SEL = 1 << 6;
        static constexpr uint8_t BTN_START = 1 << 7;
        // second byte
        static constexpr uint8_t BTN_HOME = 1 << 0;
        static constexpr uint8_t BTN_VOL_UP = 1 << 1;
        static constexpr uint8_t BTN_VOL_DOWN = 1 << 2;
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



} // namespace rckid