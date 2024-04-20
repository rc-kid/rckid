#pragma once

#include "rckid/app.h"
#include "rckid/audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    class RumblerTest : public App<FrameBuffer<ColorRGB, DisplayMode::Native_2X_RGB565>> {
    public:

        RumblerTest()  = default;

    protected:

        void update() override {
            if (pressed(Btn::A)) {
                rumblerCycles_ = (rumblerCycles_ + 1) % 9;
                if (rumblerCycles_ == 0)
                    ++rumblerCycles_;
            }
            if (pressed(Btn::B))
                exit();
            if (pressed(Btn::Up))
                rumblerStrength_ += 8;
            if (pressed(Btn::Down))
                rumblerStrength_ -= 8;
            if (pressed(Btn::Right))
                rumblerTimeOn_ += 1;
            if (pressed(Btn::Left))
                rumblerTimeOn_ -= 1;
            if (pressed(Btn::Start))
                setRumbler(RumblerEffect{rumblerStrength_, rumblerTimeOn_, RCKID_RUMBLER_FAIL_TIME_OFF, rumblerCycles_});
        }

        void draw() override {
            driver_.setFg(ColorRGB::White());
            driver_.setFont(Iosevka_Mono6pt7b);
            driver_.setBg(ColorRGB::RGB(0, 0, 0));
            driver_.fill();
            driver_.textMultiline(0,0)
                << "Rumbler Test\n\n"
                << "Strength:  " << rumblerStrength_ << "\n"
                << "Time on:   " << rumblerTimeOn_ << "\n"
                << "Cycles:    " << rumblerCycles_;
       }

    private:
        uint8_t rumblerStrength_ = RCKID_RUMBLER_DEFAULT_STRENGTH;
        uint8_t rumblerTimeOn_ = RCKID_RUMBLER_OK_TIME_ON;
        uint8_t rumblerCycles_ = 1;
    }; 
}