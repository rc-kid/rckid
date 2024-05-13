#pragma once

#include "rckid/app.h"
#include "rckid/ui/header.h"
#include "rckid/ui/carousel.h"
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
            if (down(Btn::Left))
                carousel_.prev();
            if (down(Btn::Right))
                carousel_.next();
        }

        void draw() override {
            driver_.fill();
            status_.drawOn(driver_, Rect::WH(320, 20));
            carousel_.drawOn(driver_, Rect::XYWH(0, 160, 320, 80));
        }

    private:

        Header<Color> status_;
        Carousel<Color> carousel_; 

    }; 

}