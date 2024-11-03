#pragma once

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/assets/fonts/OpenDyslexic128.h>

#include <rckid/ui/header.h>


namespace rckid {

    /** Allows setting the current date and time. 

        
     */
    class SetDateTime : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            SetDateTime app{};
            app.loop();
        }

    protected:

        SetDateTime(): GraphicsApp{Canvas<ColorRGB>{320, 240}} {
            d_ = dateTime();
        }

        /** Draws the date & time */
        void draw() override {
            g_.fill();
            std::string time{STR(now.hours() << (now.seconds() & 1 ? ":" : " ") << now.minutes())};
            int tWidth = assets::font::OpenDyslexic128::font.textWidth(time.c_str());
            g_.text(160 - tWidth / 2, 30, assets::font::OpenDyslexic128::font, color::White) << time;

            Header::drawOn(g_);
        }



        TinyDate d_;




    }; // rckid::SetDateTime


} // namespace rckid