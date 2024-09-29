#pragma once

//#include "tinytime.h"

#include <platform.h>
#include <platform/tinydate.h>
#include <platform/color_strip.h>

namespace rckid {

    static constexpr uint8_t AVR_NO_ERROR = 0;
    static constexpr uint8_t AVR_ERROR_WDT = 1;
    static constexpr uint8_t AVR_ERROR_BOD = 2;
    static constexpr uint8_t AVR_POWER_ON = 3;

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
    class State {
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

#if (defined RCKID_AVR)
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
#elif (defined ARCH_MOCK)
        void setBtnLeft(bool value) { platform::writeMask(data_[0], BTN_LEFT, value); }
        void setBtnRight(bool value) { platform::writeMask(data_[0], BTN_RIGHT, value); }
        void setBtnUp(bool value) { platform::writeMask(data_[0], BTN_UP, value); }
        void setBtnDown(bool value) { platform::writeMask(data_[0], BTN_DOWN, value); }
        void setBtnA(bool value) { platform::writeMask(data_[0], BTN_A, value); }
        void setBtnB(bool value) { platform::writeMask(data_[0], BTN_B, value); }
        void setBtnSel(bool value) { platform::writeMask(data_[0], BTN_SELECT, value); }
        void setBtnStart(bool value) { platform::writeMask(data_[0], BTN_START, value); }
        void setBtnVolUp(bool value) { platform::writeMask(data_[1], BTN_VOL_UP, value); }
        void setBtnVolDown(bool value) { platform::writeMask(data_[1], BTN_VOL_DOWN, value); }
        void setBtnHome(bool value) { platform::writeMask(data_[2], BTN_HOME, value); }
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
        uint16_t vcc() const { return voltageFromRawStorage(data_[3]); }

        static uint16_t voltageFromRawStorage(uint8_t value) {
            return value == 0 ? 0 : value + 245;
        }

#ifdef RCKID_AVR

        void setVccRaw(uint8_t raw) {
            data_[3] = raw;
        }

        static uint8_t voltageToRawStorage(uint16_t vx100) {
            if (vx100 < 250)
                return 0;
            else if (vx100 >= 500)
                return 255;
            else 
                return (vx100 - 245) & 0xff;
        }

#endif

        //@}

        /** \name Battery Voltage
         */
        //@{
        uint16_t vBatt() const { return voltageFromRawStorage(data_[4]); }

#ifdef RCKID_AVR
        void setVBattRaw(uint8_t raw) {
            data_[4] = raw;
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

        /** \name Current consumption 
         */
        //@{
        uint16_t current() const { return data_[7] * 6; }
#ifdef RCKID_AVR
        void setCurrent(uint16_t value) { 
            value = value / 6; 
            if (value > 255)
                value = 255;
            data_[7] = static_cast<uint8_t>(value); 
        }
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

        uint8_t data_[8] = {};

    } __attribute__((packed)); 

    static_assert(sizeof(State) == 8);

    struct DeviceState {
        State state; // 8
        TinyDate time; // 4
        uint32_t uptime = 0; // 4
        uint8_t error = AVR_NO_ERROR; // 1
    } __attribute__((packed));


    struct TransferrableState : public DeviceState {
        uint8_t buffer[33] = {};
    } __attribute__((packed));


    class RumblerEffect {
    public:
        uint8_t strength = 0;
        uint8_t timeOn = 0;
        uint8_t timeOff = 0;
        uint8_t cycles = 0;

        RumblerEffect() = default;

        RumblerEffect(uint8_t strength, uint8_t timeOn, uint8_t timeOff, uint8_t cycles):
            strength{strength}, timeOn{timeOn}, timeOff{timeOff}, cycles{cycles} {}

        bool active() const { return strength != 0; }


        static RumblerEffect OK() { return RumblerEffect{RCKID_RUMBLER_DEFAULT_STRENGTH, RCKID_RUMBLER_OK_TIME_ON, RCKID_RUMBLER_OK_TIME_OFF, RCKID_RUMBLER_OK_CYCLES}; }

        static RumblerEffect FAIL() { return RumblerEffect{RCKID_RUMBLER_DEFAULT_STRENGTH, RCKID_RUMBLER_FAIL_TIME_ON, RCKID_RUMBLER_FAIL_TIME_OFF, RCKID_RUMBLER_FAIL_CYCLES}; }

        static RumblerEffect Nudge() { return RumblerEffect{RCKID_RUMBLER_NUDGE_STRENGTH, RCKID_RUMBLER_NUDGE_TIME_ON, 0, 1}; }

        static RumblerEffect Off() { return RumblerEffect{}; }

    } __attribute__((packed));

    /** RGB LED effects 
     
        - Solid - display color (color, timer?)
        - Fadedisplay color, vary brightness (color, speed, min, max)
        - display color, vary hue (hue, speed)
     
     */
    class RGBEffect {
    public:
        enum class Kind : uint8_t {
            Off,
            Solid, 
            Breathe,
            Rainbow,
        }; 

        struct Rainbow {
            uint8_t hue;
            uint8_t step; 
            uint8_t brightness;
        } __attribute__((packed)); 

        /** Kind of the effect. 
         */
        Kind kind;
        /** Effect transition speed. 
         */
        uint8_t speed;
        /** Duration of the effect in seconds. 
         */
        uint8_t duration;

        union {
            platform::Color color;
            Rainbow rainbow;
        } __attribute__((packed));

        RGBEffect(): kind{Kind::Off}, speed{255}, duration{0}, color{platform::Color::Black()} {} 


        RGBEffect(RGBEffect const & from):
            kind{from.kind}, speed{from.speed}, duration{from.duration}, color{from.color} {
            if (kind == Kind::Rainbow)
                rainbow = from.rainbow;
        }

        static RGBEffect Off() { return RGBEffect{}; }

        static RGBEffect Solid(platform::Color color, uint8_t speed, uint16_t duration = 0) {
            RGBEffect result(Kind::Solid, speed, duration);
            result.color = color;
            return result;
        }

        static RGBEffect Breathe(platform::Color color, uint8_t speed, uint16_t duration = 0) {
            RGBEffect result(Kind::Breathe, speed, duration);
            result.color = color;
            return result;
        }

        static RGBEffect Rainbow(uint8_t hue, uint8_t step, uint8_t speed, uint8_t brightness = 255, uint16_t duration = 0) {
            RGBEffect result(Kind::Rainbow, speed, duration);
            result.rainbow.hue = hue;
            result.rainbow.step = step;
            result.rainbow.brightness = brightness;
            return result;
        }

        bool active() const { return kind != Kind::Off; }

        void turnOff() {
            kind = Kind::Off;
        }

        platform::Color nextColor(platform::Color const & last) {
            switch (kind) {
                case Kind::Solid:
                    // always return the 
                    return color;
                case Kind::Breathe: 
                    if (last == color)
                        return platform::Color::Black();
                    else
                        return color;
                case Kind::Rainbow:
                    rainbow.hue += rainbow.step;
                    return platform::Color::HSV(rainbow.hue * 256, 255, rainbow.brightness);
                case Kind::Off:
                default:
                    return platform::Color::Black();
            }
        }

        RGBEffect & operator = (RGBEffect const & other) {
            kind = other.kind;
            speed = other.speed;
            duration = other.duration;
            if (kind == Kind::Rainbow)
                rainbow = other.rainbow;
            else
                color = other.color;
            return *this;
        }

    private:

        RGBEffect(Kind kind, uint8_t speed, uint8_t duration):
            kind{kind}, speed{speed}, duration{duration}, color{platform::Color::Black()} {}
    } __attribute__((packed)); 

} // namespace rckid