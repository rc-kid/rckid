#pragma once
#include <new>

#include <platform.h>
#include <platform/color_strip.h>
#include <backend_config.h>

class RCKid;

namespace rckid {

    PACKED(class RGBEffect {
    public:
        enum class Kind : uint8_t {
            Off,
            Solid,
            Breathe,
            Rainbow,
        };

        using Color = platform::Color;

        PACKED(struct RainbowData {
            uint8_t hue;
            uint8_t step;
            uint8_t brightness;
        });

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
            new (this) RGBEffect{other};
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
        
        PACKED(union {
            Color color_;
            RainbowData rainbow_;
        });

        friend Writer & operator << (Writer & w, RGBEffect const & e) {
            switch (e.kind) {
                case Kind::Off:
                    w << "off";
                    return w;
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
            return w << ", speed: " << e.speed << ", duration: " << e.duration;
        }
        
    }); // RCKid::RGBEffect

    PACKED(class RumblerEffect {
    public:

        uint8_t strength = 0;
        uint8_t timeOn = 0;
        uint8_t timeOff = 0;
        uint8_t cycles = 0;

        RumblerEffect() = default;

        RumblerEffect(uint8_t strength, uint8_t timeOn, uint8_t timeOff, uint8_t cycles):
            strength{strength}, timeOn{timeOn}, timeOff{timeOff}, cycles{cycles} {}

        bool active() const { return strength != 0; }


        static RumblerEffect OK() { return RumblerEffect{RCKID_RUMBLER_OK_STRENGTH, RCKID_RUMBLER_OK_TIME_ON, RCKID_RUMBLER_OK_TIME_OFF, RCKID_RUMBLER_OK_CYCLES}; }

        static RumblerEffect Fail() { return RumblerEffect{RCKID_RUMBLER_FAIL_STRENGTH, RCKID_RUMBLER_FAIL_TIME_ON, RCKID_RUMBLER_FAIL_TIME_OFF, RCKID_RUMBLER_FAIL_CYCLES}; }

        static RumblerEffect Nudge() { return RumblerEffect{RCKID_RUMBLER_NUDGE_STRENGTH, RCKID_RUMBLER_NUDGE_TIME_ON, 0, 1}; }

        static RumblerEffect Off() { return RumblerEffect{}; }

        friend Writer & operator << (Writer & w, RumblerEffect const & e) {
            return w << "strength: " << e.strength << ", tOn: " << e.timeOn << ", tOff" <<  e.timeOff << ", cycles: " << e.cycles;
        }

    }); // rckid::RumblerEffect
    



} // namespace rckid