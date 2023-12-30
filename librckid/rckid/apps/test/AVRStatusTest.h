#pragma once

#include "rckid/app.h"
#include "rckid/audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    class AVRStatusTest : public App<FrameBuffer> {
    public:

        AVRStatusTest()  = default;

    protected:

        void update() override {
        }

        void draw() override {
            TinyDate t = time();
            Renderer & r = renderer();
            r.setFg(Color{255,255,255});
            r.setFont(Iosevka_Mono6pt7b);
            r.setBg(Color{bg_, 0, 0});
            bg_ += 4;
            r.fill();
            r.text(0,0);
            r.text() << (down(Btn::Left) ? "L " : "  ");
            r.text() << (down(Btn::Right) ? "R " : "  ");
            r.text() << (down(Btn::Up) ? "U " : "  ");
            r.text() << (down(Btn::Down) ? "D " : "  ");
            r.text() << (down(Btn::A) ? "A " : "  ");
            r.text() << (down(Btn::B) ? "B " : "  ");
            r.text() << (down(Btn::Select) ? "SEL " : "    ");
            r.text() << (down(Btn::Start) ? "START " : "      ");
            r.text() << "\n";
            r.text() << (dcPower() ? "DC " : "   ");
            r.text() << (charging() ? "CHRG " : "     ");
            r.text() << (Audio::headphones() ? "HP " : "   ");
            r.text() << vcc() << " " << tempAvr();
            r.text() << "\n";
            r.text() << t.minutes() << ":" << t.seconds();
            r.text() << "\n\n";
            r.text() << " FPS: " << fps() << " S:" << systemUs() << " U:" << (updateUs() / 1000) << " D:" << (drawUs() / 1000);
            r.text() << "\n";
            r.text() << " wU:" << ST7789::lastUpdateWaitUs() << " wS:" << ST7789::lastVSyncWaitUs() << " r: " << ST7789::lastUpdateUs();
            r.text() << "\n";
            r.text() << " idle:" << idlePct(); 
            App::draw();
        }

    private:

        uint8_t bg_ = 255;

    }; 
}