#pragma once

#include <platform/utils/writer.h>

#include "color.h"
#include "primitives.h"
#include "png.h"
#include "bitmap.h"
#include "font.h"
#include "assets/fonts/Iosevka_16.h"

namespace rckid {

    template<typename COLOR>
    class Canvas : public Bitmap<COLOR> {
    public:

        using typename Bitmap<COLOR>::Color;
        using Bitmap<COLOR>::w_;
        using Bitmap<COLOR>::h_;
        using Bitmap<COLOR>::setPixelAt;
        using Bitmap<COLOR>::pixelAt;
        using Bitmap<COLOR>::width;

        using Bitmap<COLOR>::text;
        using Bitmap<COLOR>::textMultiline;

        /** Creates new canvas by consuming already created bitmap. 
         */
        Canvas(int w, int h): 
            Bitmap<COLOR>{w, h}, 
            fg_{Color::White()}, 
            bg_{Color::Black()},
            font_{& Iosevka_16} {
        }

        Color bg() const { return bg_; }
        Color fg() const { return fg_; }

        void setFg(Color c) { fg_ = c; }
        void setBg(Color c) { bg_ = c; }

        Font const & font() const { return *font_; }
        void setFont(Font const & font) { font_ = &font; }

        Writer text(int x, int y) {
            return Bitmap<Color>::text(x, y, *font_, fg_);
        }

        Writer text(Point p) { return text(p.x, p.y); }

        Writer textMultiline(int x, int y) {
            return Bitmap<Color>::textMultiline(x, y, *font_, fg_);
        }

        using Bitmap<COLOR>::fill;

        /** Fills entire screen with the selected background color. 
         */
        void fill() { fill(Rect::WH(w_, h_)); }

        /** Fills the given rectangle with the selected background color. 
         */
        void fill(Rect rect) { fill(rect, bg_); }

        /** Draws specified rectangle with the foreground color. 
         */
        void drawRect(Rect rect) { fill(rect, fg_); }


    private:

        Color fg_;
        Color bg_;
        Font const * font_;

    }; // rckid::Canvas

    template<>
    inline void Canvas<ColorRGB>::fill() {
        uint32_t x = bg_.rawValue16() << 16 | bg_.rawValue16();
        uint32_t * b = reinterpret_cast<uint32_t*>(buffer_);
        for (unsigned i = 0, e = numPixels() / 2; i < e; ++i)
            b[i] = x;
    }

    template<>
    inline void Canvas<ColorRGB_332>::fill() {
        memset(static_cast<void*>(buffer_), bg_.rawValue8(), w_ * h_);
    }


} // namespace rckid