#pragma once

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



        using Bitmap<COLOR>::fill;

        void fill() { fill(bg_); }

        void fill(Rect rect) { fill(bg_, rect); }

        void drawRect(Rect rect) { fill(fg_, rect); }

    private:
        Color fg_;
        Color bg_;        

    }; // rckid::Canvas

    template<typename T>
    class Renderer<Canvas<T>> : public Renderer<Bitmap<T>> {};

} // namespace rckid