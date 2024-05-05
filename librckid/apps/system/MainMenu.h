#pragma once

#include "rckid/app.h"
#include "rckid/ui/Carousel.h"
#include "rckid/graphics/framebuffer.h"
#include "assets/fonts/OpenDyslexic_48.h"

namespace rckid {

    class MainMenu : public App<FrameBuffer<ColorRGB>> {
    public:

        MainMenu(Menu * menu): carousel_{menu} {
            driver_.setFont(OpenDyslexic_48);
        }

    protected:

        void update() override {
            if (pressed(Btn::Left))
                carousel_.prev();
            if (pressed(Btn::Right))
                carousel_.next();
        }

        void draw() override {
            driver_.fill();
            carousel_.drawOn(driver_, Rect::XYWH(0, 160, 320, 80));
        }

    private:

        Carousel<Color> carousel_; 

    }; 

}