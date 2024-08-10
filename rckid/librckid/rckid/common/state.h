#pragma once

//#include "tinytime.h"

#include <platform.h>
#include <platform/utils/tinydate.h>

namespace rckid {

    static constexpr uint8_t AVR_ERROR_NONE = 0;
    /// AVR has been restarted due to WDT timeout
    static constexpr uint8_t AVR_ERROR_WDT = 1;
    /// AVR has been restarted due to brown-out
    static constexpr uint8_t AVR_ERROR_BOD = 2;
    /// Charging has been disabled due to overcharge detected
    static constexpr uint8_t AVR_ERROR_VBATT_TOO_HIGH = 3;
    /// Charging has been disabled due to overtemperature
    static constexpr uint8_t AVR_ERROR_TEMP_TOO_HIGH = 4;
    /// Error in the I2C slave routine, argument is the TWI.SSTATE register
    static constexpr uint8_t AVR_ERROR_I2C_SLAVE = 5;
    /// Unknown I2C command received, argument is the command id
    static constexpr uint8_t AVR_ERROR_UNKNOWN_COMMAND = 6;
    /// assertion failure in the AVR code, argument is the assert id
    static constexpr uint8_t AVR_ERROR_ASSERT = 7;

    // custom breakpoint trigger for debugging
    static constexpr uint8_t AVR_ERROR_BREAKPOINT = 31;

    static constexpr uint8_t AVR_NO_ERROR = 0;
    static constexpr uint8_t AVR_POWER_ON = 3;

    /** RCKid mode of operation. 
     
        The device can be in one of the following modes:

        Mode       | 3V3 | Ticks | RP2040
        -----------|-----|-------|--------|
        Normal     |  x  |   x   |    x   |
        Debug      |  x  |   x   |    x   |
        Sleep      |  x  |   x   |        |
        PowerOff   |     |       |        |
        Bootloader |  x  |   x   |    x   |

     */
    enum class DeviceMode : uint8_t {
        Normal, 
        Debug,
        Sleep, 
        PowerOff, 
        Bootloader,
    }; 

    /** The smallest of the state subsets.
      
        Device state contains information about the device mode itself (normal, debug, bootloader, etc.), busy flag and basic power characteristics (DC power, charging and battery level) and user controls (all buttons as well as presence of headphones and audio (speaker & headphones detection) is enabled or not. 
     
        Furthermore, the first byte - device mode, busy flags and DC power & charging are also reported from the bootloader in the same format. 
      */
    class DeviceState {
    public:

        /** \name Device State 
         */
        //@{
        DeviceMode deviceMode() const { return static_cast<DeviceMode>(data_[STATE] & DEVICE_MODE); }
        bool debugMode() const { return deviceMode() == DeviceMode::Debug; }
        bool deviceBusy() const { return data_[STATE] & DEVICE_BUSY; }
        bool deviceError() const { return data_[STATE] & DEVICE_ERROR; }
        bool dcPower() const { return data_[STATE] & DC_PWR; }
        bool charging() const { return data_[STATE] & CHARGING; }
        //@}

#if RCKID_AVR
        void setDeviceMode(DeviceMode mode) {
            data_[STATE] &= ~DEVICE_MODE;
            data_[STATE] |= static_cast<uint8_t>(mode) & DEVICE_MODE;
        }
        void setDeviceBusy(bool value) { value ? data_[STATE] |= DEVICE_BUSY : data_[STATE] &= ~DEVICE_BUSY; }
        void setDeviceError(bool value) { value ? data_[STATE] |= DEVICE_ERROR : data_[STATE] &= ~DEVICE_ERROR; }
        void setDCPower(bool value) { value ? data_[STATE] |= DC_PWR : data_[STATE] &= ~DC_PWR; }
        void setCharging(bool value) { value ? data_[STATE] |= CHARGING : data_[STATE] &= ~CHARGING; }
#endif

        /** \name Controls
         */
        //@{
        bool btnLeft() const { return data_[CONTROLS1] & BTN_LEFT; }
        bool btnRight() const { return data_[CONTROLS1] & BTN_RIGHT; }
        bool btnUp() const { return data_[CONTROLS1] & BTN_UP; }
        bool btnDown() const { return data_[CONTROLS1] & BTN_DOWN; }
        bool btnA() const { return data_[CONTROLS1] & BTN_A; }
        bool btnB() const { return data_[CONTROLS1] & BTN_B; }
        bool btnSel() const { return data_[CONTROLS1] & BTN_SELECT; }
        bool btnStart() const { return data_[CONTROLS1] & BTN_START; }
        bool btnHome() const { return data_[CONTROLS2] & BTN_HOME; }
        bool btnVolUp() const { return data_[CONTROLS2] & BTN_VOL_UP; }
        bool btnVolDown() const { return data_[CONTROLS2] & BTN_VOL_DOWN; }

        bool headphones() const { return data_[CONTROLS2] & HEADPHONES; }
        bool audioEnabled() const { return data_[CONTROLS2] & AUDIO_EN; }

#if (defined RCKID_AVR)
        void setDPadKeys(bool l, bool r, bool u, bool d) {
            data_[CONTROLS1] &= ~(BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN);
            data_[CONTROLS1] |= (l ? BTN_LEFT : 0) | ( r ? BTN_RIGHT : 0) | (u ? BTN_UP : 0) | (d ? BTN_DOWN : 0);
        }
        void setABXYKeys(bool a, bool b, bool sel, bool start) {
            data_[CONTROLS1] &= ~(BTN_A | BTN_B | BTN_SELECT | BTN_START);
            data_[CONTROLS1] |= (a ? BTN_A : 0) | ( b ? BTN_B : 0) | (sel ? BTN_SELECT : 0) | (start ? BTN_START : 0);
        }
        void setBtnHome(bool value = true) { value ? data_[CONTROLS2] |= BTN_HOME : data_[CONTROLS2] &= ~BTN_HOME; }
        void setVolumeKeys(bool volUp, bool volDown) {
            data_[CONTROLS2] &= ~(BTN_VOL_UP | BTN_VOL_DOWN);
            data_[CONTROLS2] |= ( volUp ? BTN_VOL_UP : 0) | (volDown ? BTN_VOL_DOWN : 0);
        }
        void setHeadphones(bool value) { value ? data_[CONTROLS2] |= HEADPHONES : data_[CONTROLS2] &= ~HEADPHONES; }
        void setAudioEnabled(bool value) { value ? data_[CONTROLS2] |= AUDIO_EN : data_[CONTROLS2] &= ~AUDIO_EN; }
#elif (defined ARCH_MOCK)
        void setBtnLeft(bool value) { platform::writeMask(data_[CONTROL1], BTN_LEFT, value); }
        void setBtnRight(bool value) { platform::writeMask(data_[CONTROL1], BTN_RIGHT, value); }
        void setBtnUp(bool value) { platform::writeMask(data_[CONTROLS1], BTN_UP, value); }
        void setBtnDown(bool value) { platform::writeMask(data_[CONTROLS1], BTN_DOWN, value); }
        void setBtnA(bool value) { platform::writeMask(data_[CONTROLS1], BTN_A, value); }
        void setBtnB(bool value) { platform::writeMask(data_[CONTROLS1], BTN_B, value); }
        void setBtnSel(bool value) { platform::writeMask(data_[CONTROLS1], BTN_SELECT, value); }
        void setBtnStart(bool value) { platform::writeMask(data_[CONTROLS1], BTN_START, value); }
        void setBtnHome(bool value) { platform::writeMask(data_[CONTROLS1], BTN_HOME, value); }
        void setBtnVolUp(bool value) { platform::writeMask(data_[CONTROLS1], BTN_VOL_UP, value); }
        void setBtnVolDown(bool value) { platform::writeMask(data_[CONTROLS1], BTN_VOL_DOWN, value); }
#endif
        //@}

        /** \name Battery Voltage
         */
        //@{
        uint16_t vBatt() const { return voltageFromRawStorage(data_[VBATT]); }

#ifdef RCKID_AVR

        void setVBattRaw(uint8_t raw) {
            data_[VBATT] = raw;
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

    private:

        friend class ExtendedState;

        static constexpr uint8_t STATE = 0;
        static constexpr uint8_t DEVICE_MODE = 7; // 3
        static constexpr uint8_t DEVICE_BUSY = 1 << 3;
        static constexpr uint8_t DEVICE_ERROR = 1 << 4;
        static constexpr uint8_t DC_PWR = 1 << 5;
        static constexpr uint8_t CHARGING = 1 << 6;

        static constexpr uint8_t CONTROLS1 = 1;
        static constexpr uint8_t BTN_LEFT = 1 << 0;
        static constexpr uint8_t BTN_RIGHT = 1 << 1;
        static constexpr uint8_t BTN_UP = 1 << 2;
        static constexpr uint8_t BTN_DOWN = 1 << 3;
        static constexpr uint8_t BTN_A = 1 << 4;
        static constexpr uint8_t BTN_B = 1 << 5;
        static constexpr uint8_t BTN_SELECT = 1 << 6;
        static constexpr uint8_t BTN_START = 1 << 7;

        static constexpr uint8_t CONTROLS2 = 2;
        static constexpr uint8_t BTN_HOME = 1 << 0;
        static constexpr uint8_t BTN_VOL_UP = 1 << 1;
        static constexpr uint8_t BTN_VOL_DOWN = 1 << 2;
        static constexpr uint8_t HEADPHONES = 1 << 3;
        static constexpr uint8_t AUDIO_EN = 1 << 4;

        static constexpr uint8_t VBATT = 3;

        static uint16_t voltageFromRawStorage(uint8_t value) {
            return value == 0 ? 0 : value + 245;
        }

        uint8_t data_[4] = {};

    } __attribute__((packed)); 

    static_assert(sizeof(DeviceState) == 4);

    /** Extended state containing information about measured VCC level, temperature, current and brightness. Similarly to the DeviceState, only the AVR can change the state via I2C commands. 
     */
    class ExtendedState {
    public:

        /** VCC
         
            The voltage to the AVR measured in 0.01[V]. Value of 0 means any voltage below 2.46V, value of 500 5V or more, otherwise the number returnes is volatge * 100. 
         */
        uint16_t vcc() const { return DeviceState::voltageFromRawStorage(vcc_); }

        /** Temperature 
         
            Returns the temperature as measured by the chip with 0.1[C] intervals. -200 is -20C or less, 1080 is 108[C] or more. 0 is 0C. 
         */
        int16_t temp() const { return -200 + (temp_ * 5); }

        /** Current consumption 
         */
        uint16_t current() const { return current_ * 6; }

        uint8_t brightness() const { return brightness_; }

#ifdef RCKID_AVR
        void setVccRaw(uint8_t raw) { vcc_ = raw; }

        void setTemp(int32_t tempx10) {
            if (tempx10 <= -200)
                temp_ = 0;
            else if (tempx10 >= 1080)
                temp_ = 255;
            else 
                temp_ = (tempx10 + 200) / 5;
        }

        void setCurrent(uint16_t value) { 
            value = value / 6; 
            if (value > 255)
                value = 255;
            current_ = static_cast<uint8_t>(value); 
        }

        void setBrightness(uint8_t value) { brightness_ = value; }

#endif

    private:
        uint8_t vcc_;
        uint8_t temp_;
        uint8_t current_;
        uint8_t brightness_;
    } __attribute__((packed)); 

    static_assert(sizeof(ExtendedState) == 4);

    /** Contains the full state, which consists of the device state itself, as well as extra information that is not necessary for basic functionality and as such is rarely questioned, such as temperature, VCC level, current consumption, time, etc.
     */
    class FullState {
    public:
        DeviceState device; // 4
        ExtendedState estate; // 4
        TinyDate time; // 4
        uint32_t uptime = 0; // 4
    } __attribute__((packed));

    static_assert(sizeof(FullState) == 16);

    /** A transferrable state consists of the full state, followed by a 33 byte long buffer, i.e. large enough to store a single command byte and 32byte payload. The buffer is used to (a) store commands and their payloads sent from RP2040 to the device and to return arbitrary data back to the master (some commands then populate the buffer payload portion with new data of their own). 
     */
    struct TransferrableState : public FullState {
    public:
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

        static RGBEffect Solid(platform::Color color, uint8_t speed = 1, uint16_t duration = 0) {
            RGBEffect result(Kind::Solid, speed, duration);
            result.color = color;
            return result;
        }

        static RGBEffect Breathe(platform::Color color, uint8_t speed = 1, uint16_t duration = 0) {
            RGBEffect result(Kind::Breathe, speed, duration);
            result.color = color;
            return result;
        }

        static RGBEffect Rainbow(uint8_t hue, uint8_t step = 1, uint8_t speed = 1, uint8_t brightness = 255, uint16_t duration = 0) {
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

        /** Returns true if the effect is visually the same, so that effects don't get reassigned unless necessary. Effects are the same regardless their duration (which should always be extended) and in case of rainbows, their hue.
         */
        bool isSameAs(RGBEffect const & other) const {
            if (other.kind != kind)
                return false;
            if (other.speed != speed)
                return false;
            if (kind != Kind::Rainbow && color != other.color)
                return false;
            return true;
        }

    private:

        RGBEffect(Kind kind, uint8_t speed, uint8_t duration):
            kind{kind}, speed{speed}, duration{duration}, color{platform::Color::Black()} {}
    } __attribute__((packed)); 


    namespace cmd {

        enum class Id : uint8_t {
            #define COMMAND(ID, NAME, ...) NAME ID, 
            #include "commands.inc.h"
        }; 

        inline Id getIdFrom(uint8_t const * buffer) {
            if (*buffer < 0x80)
                return static_cast<Id>(*buffer & 0b11000000);
            else 
               return static_cast<Id>(*buffer);
        }

#define COMMAND(ID_HINT, NAME, ...)                         \
    class NAME {                                                 \
    protected:  \
        uint8_t id_ = static_cast<uint8_t>(Id::NAME); \
    public:                                                      \
        static Id constexpr ID = Id::NAME;                       \
        static NAME const & fromBuffer(uint8_t const * buffer) { \
            return * reinterpret_cast<NAME const *>(buffer);     \
        }     \
        Id id() const { return getIdFrom(reinterpret_cast<uint8_t const*>(this)); }           \
        __VA_ARGS__                                              \
    } __attribute__((packed));                                   \
    static_assert(sizeof(NAME) <= 32);              

#include "commands.inc.h"

    } // radio::msg


} // namespace rckid