#pragma once

#include "rckid/app.h"

namespace rckid {

    class RumblerTest : public App<FrameBuffer<ColorRGB>> {
    public:
        static RumblerTest * create() { return new RumblerTest{}; }

    protected:

        void onFocus() override {
            App::onFocus();
            setRumbler(RumblerEffect::Off());
        }

        void onBlur() override {
            App::onFocus();
            setRumbler(RumblerEffect::Off());
        } 

        void update() override {
            App::update();
            if (pressed(Btn::Up))
                strength_ += 16;
            else if (pressed(Btn::Down))
                strength_ -= 16;
            else if (pressed(Btn::Left))
                time_ -= 10;
            else if (pressed(Btn::Right))
                time_ += 10;
            else if (pressed(Btn::A)) {
                gpio::high(GPIO14);
                setRumbler(RumblerEffect{strength_, time_, time_, 3});
                gpio::low(GPIO14);
            }
            else if (pressed(Btn::Select))
                setRumbler(RumblerEffect::Nudge());
            else if (pressed(Btn::Start))
                setRumbler(RumblerEffect::OK());
        }

        void draw() override {
            driver_.fill();
            driver_.textMultiline(10, 20) <<
                "Strength:       " << (int32_t)strength_ << "\n" <<
                "Time:           " << (int32_t)time_ << "\n";
        }

        uint8_t strength_ = RCKID_RUMBLER_DEFAULT_STRENGTH;
        uint8_t time_ = RCKID_RUMBLER_OK_TIME_ON;

    }; // rckid::SerialMonitor

} // namespace rckid