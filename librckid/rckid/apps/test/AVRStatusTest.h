#pragma once

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"


namespace rckid {

    class AVRStatusTest : public FBApp<FrameBuffer<ColorRGB>> {
    public:

        AVRStatusTest()  = default;

    protected:

        void onFocus() override {
            FBApp::onFocus();
            setBrightness(32);
        }

        void update() override {
            if (pressed(Btn::A))
                setBrightness(254);
            if (pressed(Btn::B)) {
               setBrightness(32);
               FATAL_ERROR(45);
            }
        }

        void draw() override {
            gpio::high(GPIO21);
            TinyDate t = time();
            fb_.setFg(Color::White());
            fb_.setFont(Iosevka_Mono6pt7b);
            fb_.setBg(Color::RGB(bg_, 0, 0));
            bg_ += 4;
            fb_.fill();
            fb_.textMultiline(0,0) << (down(Btn::Left) ? "L " : "  ")
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
                        << " FPS: " << stats::fps() << " S:" << stats::systemUs() << " U:" << (stats::updateUs() / 1000) << " D:" << (stats::drawUs() / 1000)
                        << "\n"
                        << " wU:" << stats::lastUpdateWaitUs() << " wS:" << stats::lastVSyncWaitUs() << " r: " << stats::lastUpdateUs()
                        << "\n"
                        << " idle:" << stats::idlePct() << "\n"
                        << " VRAM: " << freeVRAM() << "\n"
                        << " heap: " << freeHeap();
            gpio::low(GPIO21);

        }

    private:

        uint8_t bg_ = 255;

    }; 
}