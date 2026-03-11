#pragma once

#include <platform.h>
#include <platform/writer.h>
#include <platform/color_strip.h>

class RCKid;

namespace rckid {

    enum class PowerMode {
        Normal,
        Boost,
    }; // rckid::PowerMode

    enum class Btn {
        Left       = 0, 
        Right      = 1,
        Up         = 2, 
        Down       = 3, 
        A          = 4, 
        B          = 5, 
        Select     = 6, 
        Start      = 7,
        Home       = 8, 
        VolumeUp   = 9, 
        VolumeDown = 10,
    }; // rckid::Btn

    [[noreturn]] void onFatalError(char const * file, uint32_t line, char const * msg, uint32_t payload);

    /** Represents the basic device state. 
     
        The state holds information about buttons and power state. This is a short value that is expected to be read very often (every tick) and contains all of the information needed to drive the basic functionality. 

        The first byte corresponds to the current state of the 8 control buttons (dpad, a, b, sel and start). 

        The second byte corresponds to side buttons (home, vol up and down) and device states (debug mode, parent mode, chargingm headphones, etc.)

        Third byte is current voltage. 

        Fourth byte is interrupts and is cleared specifically

        - 8 bits for dpad, a, b, sel, start buttons
        
        - 3 bits for home, vol up & down buttons
        - 1 bit wakeup interrupt
        - 1 bit accel interrupt
        - 1 bit poweroff interrupt
        - 1 bit second interrupt
        - 

        - 1 bit debug mode
        - 1 bit parent mode
        - 1 bit bootloader mode
        - 1 bit charging
        - 
        -
        -
        - 1 bit headphones connected

        - 7 bits for battery voltage level
        - 1 bit for usb power connected
     */
    class DeviceState {
    public:

        /** Updates the device state with new state value. The operation is slightly more involved as the interrupt flags must be ORed in. 
         */
        void updateWith(DeviceState const & other) {
            a_ = other.a_;
            b_ = other.b_;
            c_ |= other.c_;
            d_ = other.d_;
        }

        void clearInterrupts() {
            c_ = 0;
        }

        bool button(Btn btn) {
            uint8_t btnId = static_cast<uint8_t>(btn);
            if (btnId >= 8)
                return (b_ & (1 << (btnId - 8))) != 0;
            else
                return (a_ & (1 << btnId)) != 0;
        }

        /** Sets the value of given button. Returns true if the state changed, false otherwise.
         */
        bool setButton(Btn btn, bool state) {
            uint8_t btnId = static_cast<uint8_t>(btn);
            if (button(btn) == state)
                return false;
            if (btnId >= 8) {
                btnId -= 8;
                state ? (b_ |= (1 << btnId)) : (b_ &= ~(1 << btnId));
            } else {
                state ? (a_ |= (1 << btnId)) : (a_ &= ~(1 << btnId));
            }
            return true;
        }

        bool headphonesConnected() const { return (b_ & HEADPHONES_MASK) != 0; }
        bool charging() const { return (b_ & CHARGING_MASK) != 0; }
        bool debugMode() const { return (b_ & DEBUG_MODE_MASK) != 0; }


        void setHeadphonesConnected(bool connected) { connected ? (b_ |= HEADPHONES_MASK) : (b_ &= ~HEADPHONES_MASK); }
        void setCharging(bool charging) { charging ? (b_ |= CHARGING_MASK) : (b_ &= ~CHARGING_MASK); }
        void setDebugMode(bool debugMode) { debugMode ? (b_ |= DEBUG_MODE_MASK) : (b_ &= ~DEBUG_MODE_MASK); }

        bool wakeUpInterrupt() const { return (c_ & WAKEUP_INTERRUPT_MASK) != 0; }
        void setWakeUpInterrupt(bool requested) { requested ? (c_ |= WAKEUP_INTERRUPT_MASK) : (c_ &= ~WAKEUP_INTERRUPT_MASK); }
        
        bool accelInterrupt() const { return (c_ & ACCEL_INTERRUPT_MASK) != 0; }
        void setAccelInterrupt(bool requested) { requested ? (c_ |= ACCEL_INTERRUPT_MASK) : (c_ &= ~ACCEL_INTERRUPT_MASK); }

        bool powerOffInterrupt() const { return (c_ & POWER_OFF_INTERRUPT_MASK) != 0; }
        void setPowerOffInterrupt(bool requested) { requested ? (c_ |= POWER_OFF_INTERRUPT_MASK) : (c_ &= ~POWER_OFF_INTERRUPT_MASK); }


        /** Returns the current voltage in 100mV units (i.e. volts * 10). 

            The available range is 2.45V (250) to 5V (500). Anything higher than 5V will still read as 5V and anything below 2.45V will read as 0V. This is fine as the range is only used to determine li-ion batter levels (3.00V - 4.30V), or USB power plugged in (5.00V).
         */
        uint32_t vcc() const { return (d_ == 0) ? 0 : (d_ + 245); }

        void setVcc(uint16_t vx100) {
            if (vx100 < 245)
                d_ = 0;
            else if (vx100 > 500)
                d_ = 255;
            else
                d_ = static_cast<uint8_t>(vx100 - 245);
        }

    private:
        /** MSB ....... LSB
            | | | | | | | |
            | | | | | | | --- dpad left
            | | | | | | ----- dpad right
            | | | | | ------- dpad up
            | | | | --------- dpad down
            | | | ----------- a
            | | ------------- b
            | --------------- select
            ----------------- start 
         */
        uint8_t a_ = 0; 
        /** MSB ....... LSB
            | | | | | | | |
            | | | | | | | --- home
            | | | | | | ----- volume up
            | | | | | ------- volume down
            | | | | --------- headphones connected
            | | | ----------- charging
            | | ------------- debug mode
            | --------------- 
            -----------------  
         */
        uint8_t b_ = 0; 
        static constexpr uint8_t HEADPHONES_MASK = 1 << 3;
        static constexpr uint8_t CHARGING_MASK = 1 << 4;
        static constexpr uint8_t DEBUG_MODE_MASK = 1 << 5;
        /** MSB ....... LSB (interrupts)
            | | | | | | | |
            | | | | | | | --- 
            | | | | | | ----- 
            | | | | | ------- 
            | | | | --------- 
            | | | ----------- 
            | | ------------- wakeup interrupt
            | --------------- accel interrupt
            ----------------- power off interrupt 
         */
        uint8_t c_ = 0;
        static constexpr uint8_t WAKEUP_INTERRUPT_MASK = 1 << 6;
        static constexpr uint8_t ACCEL_INTERRUPT_MASK = 1 << 6;
        static constexpr uint8_t POWER_OFF_INTERRUPT_MASK = 1 << 7;
        /** Voltage 
         */
        uint8_t d_ = 0;

    }; // rckid::hal::State

    static_assert(sizeof(DeviceState) == 4, "Required so that State can be read/written as a single 32bit value");



    class RGBEffect {
    public:
        enum class Kind : uint8_t {
            Off,
            Solid,
            Breathe,
            Rainbow,
        };

        using Color = platform::Color;

        struct RainbowData {
            uint8_t hue;
            uint8_t step;
            uint8_t brightness;
        } __attribute__((packed));

        Kind kind;
        /** Speeds 0..7 means that this many frames will be the duration single step of the effect, while larger speeds change every frame, but by an increased delta.
         */
        uint8_t speed;
        uint8_t duration;

        void setColor(Color color) {
            ASSERT(kind != Kind::Rainbow);
            color_ = color;
        }

        RGBEffect(): RGBEffect{Kind::Off, 0, 0} {}

        RGBEffect(RGBEffect const & other):
            kind{other.kind},
            speed{other.speed},
            duration{other.duration} {
            if (kind == Kind::Rainbow) 
                rainbow_ = other.rainbow_;
            else 
                color_ = other.color_;            
        }

        RGBEffect & operator = (RGBEffect const & other) {
            kind = other.kind;
            speed = other.speed;
            duration = other.duration;
            if (kind == Kind::Rainbow)
                rainbow_ = other.rainbow_;
            else
                color_ = other.color_;
            return *this;
        }

        static RGBEffect Off() {
            return RGBEffect{Kind::Off, 0, 0};
        }

        static RGBEffect Solid(Color color, uint8_t speed = 8, uint8_t duration = 0) {
            RGBEffect result{Kind::Solid, speed, duration};
            result.setColor(color);
            return result;
        }

        static RGBEffect Breathe(Color color, uint8_t speed = 8, uint8_t duration = 0) {
            RGBEffect result{Kind::Breathe, speed, duration};
            result.setColor(color);
            return result;
        }

        static RGBEffect Rainbow(uint8_t hue, uint8_t step, uint8_t speed = 8, uint8_t brightness = 255, uint16_t duration = 0) {
            RGBEffect result(Kind::Rainbow, speed, duration);
            result.rainbow_.hue = hue;
            result.rainbow_.step = step;
            result.rainbow_.brightness = brightness;
            return result;
        }

        bool active() const { return kind != Kind::Off; }

        void turnOff() {
            kind = Kind::Off;
        }

        uint8_t skipTicks() const {
            // for speeds 0..7, we skip speed - 1 ticks, for larger speeds we do not skip any ticks
            if (speed <= 7)
                return 8 - speed;
            else
                return 0;
        }

        uint8_t changeDelta() const {
            // for speeds 0..7, we change by 1 each frame, for larger speeds we change by speed - 7 each frame
            if (speed <= 7)
                return 1;
            else
                return speed - 7;
        }

    private:

        friend class ::RCKid;

        RGBEffect(Kind kind, uint8_t speed, uint8_t duration):
            kind{kind},
            speed{speed},
            duration{duration} {
            color_ = Color::Black();
        }

        Color nextColor(Color const & last) {
            switch (kind) {
                case Kind::Solid:
                    // always return the 
                    return color_;
                case Kind::Breathe: 
                    if (last == color_)
                        return Color::Black();
                    else
                        return color_;
                case Kind::Rainbow:
                    rainbow_.hue += rainbow_.step;
                    return Color::HSV(rainbow_.hue * 256, 255, rainbow_.brightness);
                case Kind::Off:
                default:
                    return Color::Black();
            }
        }
        
        union {
            Color color_;
            RainbowData rainbow_;
        } __attribute__((packed));

        friend void write(Writer & w, RGBEffect const & e) {
            switch (e.kind) {
                case Kind::Off:
                    w << "off";
                    return;
                case Kind::Solid:
                    w << "solid: " << hex(e.color_.r, false) << hex(e.color_.g, false) << hex(e.color_.b, false);
                    break;
                case Kind::Breathe:
                    w << "breathe: " << hex(e.color_.r, false) << hex(e.color_.g, false) << hex(e.color_.b, false);
                    break;
                case Kind::Rainbow:
                    w << "rainbow: hue: " << e.rainbow_.hue << ", step: " << e.rainbow_.step << ", brightness: " << e.rainbow_.brightness;
                    break;
            }
            w << ", speed: " << e.speed << ", duration: " << e.duration;
        }

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

        static RumblerEffect Off() { return RumblerEffect{}; }

        friend void write(Writer & w, RumblerEffect const & e) {
            w << "strength: " << e.strength << ", tOn: " << e.timeOn << ", tOff" <<  e.timeOff << ", cycles: " << e.cycles;
        }

    } __attribute__((packed)); // rckid::RumblerEffect



} // namespace rckid