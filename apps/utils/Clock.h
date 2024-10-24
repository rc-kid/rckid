#pragma once

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/assets/fonts/OpenDyslexic128.h>

#include <rckid/ui/header.h>

namespace rckid {

    class Clock : public GraphicsApp<Canvas<ColorRGB>> {
    public:
        static void run() {
            Clock app{};
            app.loop();
        }

    protected:

        Clock(): GraphicsApp{Canvas<ColorRGB>{320, 240}} {}

        void draw() override {
            g_.fill();
            std::string time = "10:45";
            g_.text(100, 30, assets::font::OpenDyslexic128::font, color::White) << time;



            Header::drawOn(g_);
        }

    }; // rckid::Clock
} // namespace rckid
