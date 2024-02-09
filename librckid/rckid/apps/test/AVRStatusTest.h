#pragma once

#include "rckid/app.h"
#include "rckid/audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    class AVRStatusTest : public App<FrameBuffer<ColorRGB>> {
    public:

        AVRStatusTest()  = default;

    protected:

        void update() override {
        }

        void draw() override {
            TinyDate t = time();
            Renderer & r = renderer();
            r.setFg(ColorRGB::White());
            r.setFont(Iosevka_Mono6pt7b);
            r.setBg(ColorRGB::RGB(bg_, 0, 0));
            bg_ += 4;
            r.fill();
            r.text(0,0) << (down(Btn::Left) ? "L " : "  ")
                        << (down(Btn::Right) ? "R " : "  ")
                        << (down(Btn::Up) ? "U " : "  ")
                        << (down(Btn::Down) ? "D " : "  ")
                        << (down(Btn::A) ? "A " : "  ")
                        << (down(Btn::B) ? "B " : "  ")
                        << (down(Btn::Select) ? "SEL " : "    ")
                        << (down(Btn::Start) ? "START " : "      ")
                        << "\n"
                        << (dcPower() ? "DC " : "   ")
                        << (charging() ? "CHRG " : "     ")
                        << (Audio::headphones() ? "HP " : "   ")
                        << vcc() << " " << tempAvr()
                        << "\n"
                        << t.minutes() << ":" << t.seconds()
                        << "\n\n"
                        << " FPS: " << Stats::fps() << " S:" << Stats::systemUs() << " U:" << (Stats::updateUs() / 1000) << " D:" << (Stats::drawUs() / 1000)
                        << "\n"
                        << " wU:" << Stats::lastUpdateWaitUs() << " wS:" << Stats::lastVSyncWaitUs() << " r: " << Stats::lastUpdateUs()
                        << "\n"
                        << " idle:" << Stats::idlePct();
        }

    private:

        uint8_t bg_ = 255;

    }; 
}