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

        PACKED(struct Rainbow {
            uint8_t hue;
            uint8_t step;
            uint8_t brightness;
        });

        Kind kind;
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

        static RGBEffect Solid(Color color, uint8_t speed = 1, uint8_t duration = 0) {
            RGBEffect result{Kind::Solid, speed, duration};
            result.setColor(color);
            return result;
        }

        static RGBEffect Breathe(Color color, uint8_t speed, uint8_t duration = 0) {
            RGBEffect result{Kind::Breathe, speed, duration};
            result.setColor(color);
            return result;
        }

        bool active() const { return kind != Kind::Off; }

        void turnOff() {
            kind = Kind::Off;
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
            Rainbow rainbow_;
        });
        
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


        static RumblerEffect OK() { return RumblerEffect{RCKID_RUMBLER_DEFAULT_STRENGTH, RCKID_RUMBLER_OK_TIME_ON, RCKID_RUMBLER_OK_TIME_OFF, RCKID_RUMBLER_OK_CYCLES}; }

        static RumblerEffect FAIL() { return RumblerEffect{RCKID_RUMBLER_DEFAULT_STRENGTH, RCKID_RUMBLER_FAIL_TIME_ON, RCKID_RUMBLER_FAIL_TIME_OFF, RCKID_RUMBLER_FAIL_CYCLES}; }

        static RumblerEffect Nudge() { return RumblerEffect{RCKID_RUMBLER_NUDGE_STRENGTH, RCKID_RUMBLER_NUDGE_TIME_ON, 0, 1}; }

        static RumblerEffect Off() { return RumblerEffect{}; }


    }); // rckid::RumblerEffect
    



} // namespace rckid