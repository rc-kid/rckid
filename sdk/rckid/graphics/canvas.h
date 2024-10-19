#pragma once

#include "../assets/fonts/Iosevka16.h"
#include "bitmap.h"

namespace rckid {

    template<typename COLOR>
    class Canvas : public Bitmap<COLOR> {
    public:

        using Color = COLOR;

        Canvas(Coord width, Coord height): Bitmap<Color>{width, height} {}

        Color fg() const { return fg_; }
        void setFg(Color value) { fg_ = value; }

        Color bg() const { return bg_; }
        void setBg(Color value) { bg_ = value; }

        Font const & font() const { return *font_; }
        void setFont(Font const & font) { font_ = & font; }

        using Bitmap<COLOR>::fill;

        void fill() { fill(bg_); }

        void fill(Rect rect) { fill(bg_, rect); }

        void drawRect(Rect rect) { fill(fg_, rect); }

        Writer text(int x, int y) {
            return Bitmap<Color>::text(x, y, *font_, fg_);
        }

        Writer text(Point p) { return text(p.x, p.y); }

        using Bitmap<COLOR>::text;

    private:
        Color fg_;
        Color bg_;   
        Font const * font_ = & defaultFont_;   

        static constexpr Font defaultFont_{Font::fromROM<assets::font::Iosevka16>()};  

    }; // rckid::Canvas

    template<typename T>
    class Renderer<Canvas<T>> : public Renderer<Bitmap<T>> {};

    template<>
    inline Canvas<ColorRGB>::Canvas(Coord width, Coord height): 
        Bitmap<Color>{width, height},
        fg_{color::White}, 
        bg_{color::Black} {
    }


} // namespace rckid