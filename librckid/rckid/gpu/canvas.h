#pragma once

#include "../rckid.h"
#include "../writer.h"
#include "graphics.h"
#include "gfx.h"

namespace rckid {

    class Canvas {
    public:

        Canvas(uint16_t width, uint16_t height):
            buffer_{ new Color[ (unsigned)width * height] },
            w_{width},
            h_{height} {
        }

        void pixel(int x, int y) { pixel(x, y, fg_); }
        void pixel(int x, int y, Color c) { buffer_[toOffset(x, y)] = c; }

        int text(int x, int y, char const * str);

        Writer text(int x, int y) {
            x_ = x;
            y_ = y;
            return text();
        }

        Writer text() {
            return Writer([this](char c){
                x_ += drawGlyph(x_, y_, c, fg_, font_, 1);
                if (x_ >= w_ || c == '\n') {
                    x_ = 0;
                    y_ += font_->yAdvance;
                }
            });
        }

        void setFg(Color c) { fg_ = c; }
        void setBg(Color c) { bg_ = c; }
        void setFont(GFXfont const & font) { font_ = &font; }


        Color const * rawPixels() const { return buffer_; }
        size_t rawPixelsCount() const { return w_ * h_; }
    private:

        size_t toOffset(int x, int y) { /*return x * h_ + y; */ return y * w_ + x; }

        int drawGlyph(int x, int y, char glyph, Color c, GFXfont const * f, int size);


        Color fg_;
        Color bg_;
        GFXfont const * font_;
        uint16_t x_;
        uint16_t y_;
        
        Color * buffer_;
        unsigned w_;
        unsigned h_;

    }; // rckid::Canvas

} // namespace rckid