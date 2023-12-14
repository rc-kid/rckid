#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"



namespace rckid {

    class AVRStatusTest : public App<Framebuffer<display_profile::RGB>> {
    public:

        AVRStatusTest()  = default;

        AVRStatusTest(App * parent): App{parent} { }

    protected:

        void update() override {
            i2c_read_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t *)& state_, sizeof(State), false);

        }

        void draw() {
            Renderer & r = renderer();
            r.setFg(Color{255,255,255});
            r.setFont(Iosevka_Mono6pt7b);
            r.setBg(Color{bg_, 0, 0});
            bg_ += 4;
            r.fill();
            r.text(0,0);
            r.text() << (state_.status.dpadLeft() ? "L " : "  ");
            r.text() << (state_.status.dpadRight() ? "R " : "  ");
            r.text() << (state_.status.dpadUp() ? "U " : "  ");
            r.text() << (state_.status.dpadDown() ? "D " : "  ");
            r.text() << (state_.status.btnA() ? "A " : "  ");
            r.text() << (state_.status.btnB() ? "B " : "  ");
            r.text() << (state_.status.btnSelect() ? "SEL " : "    ");
            r.text() << (state_.status.btnStart() ? "START " : "      ");
            r.text() << "\n";
            r.text() << (state_.status.dcPower() ? "DC " : "   ");
            r.text() << (state_.status.charging() ? "CHRG " : "     ");
            r.text() << (state_.status.headphones() ? "HP " : "   ");
            r.text() << state_.info.vcc() << " " << state_.info.temp();
            r.text() << "\n";
            r.text() << state_.time.minutes() << ":" << state_.time.seconds() << " bright:" << state_.config.backlight();            
            r.text() << "\n\n";
            r.text() << " FPS: " << fps() << " S:" << systemUs() << " U:" << (updateUs() / 1000) << " D:" << (drawUs() / 1000);
            r.text() << "\n";
            r.text() << " wU:" << ST7789::lastUpdateWaitUs() << " wS:" << ST7789::lastVSyncWaitUs() << " r: " << ST7789::lastUpdateUs();
            r.text() << "\n";
            r.text() << " idle:" << idlePct(); 
        }

    private:

        State state_;
        uint8_t bg_ = 255;

    }; 
}