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
            Font const & f = assets::font::OpenDyslexic128::font;
            TinyDate now = dateTime();
            std::string h{STR(fillLeft(now.hour(), 2, '0'))};
            std::string m{STR(fillLeft(now.minute(), 2, '0'))};
            int hWidth = f.textWidth(h.c_str());
            g_.text(150 - hWidth, 30, f, color::White) << h;
            g_.text(170, 30, f, color::White) << m;
            if (now.second() & 1)
                g_.text(160 - f.glyphInfoFor(':').advanceX / 2, 30, f, color::White) << ':';


            // TODO draw alarm clock as well and allow its setting
            Header::drawOn(g_);
        }

    }; // rckid::Clock
} // namespace rckid
