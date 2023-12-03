#pragma once

#include "rckid/color.h"
#include "rckid/writer.h"

#include "gfx.h"

namespace rckid {

    template<typename PIXEL_FORMAT>
    class Canvas {
    public:
        using Color = PIXEL_FORMAT;

        Canvas(int width, int height):
            buffer_{ new Color[ width * height] },
            w_{width},
            h_{height} {
        }

        ~Canvas() { 
            delete [] buffer_;
        }

        void setFg(Color c) { fg_ = c; }
        void setBg(Color c) { bg_ = c; }
        void setFont(GFXfont const & font) { font_ = &font; }

        void pixel(int x, int y) { pixel(x, y, fg_); }
        void pixel(int x, int y, Color c) { buffer_[map(x, y)] = c; }

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

        void fill() {
            for (unsigned i = 0, e = rawPixelsCount(); i < e; ++i)
                buffer_[i] = bg_;
        }

        Color const * rawPixels() const { return buffer_; }
        size_t rawPixelsCount() const { return w_ * h_; }

    private:

        size_t map(int x, int y) { return (w_ - x -1) * h_ + y; }

        int drawGlyph(int x, int y, char c, Color color, GFXfont const * f, int size) {
            GFXglyph * glyph = f->glyph + (c - f->first);
            uint8_t const * bitmap = f->bitmap + glyph->bitmapOffset;
            int pixelY = y + glyph->yOffset * size + f->yAdvance;
            int bi = 0;
            uint8_t bits;
            for (int gy = 0; gy < glyph->height; ++gy, pixelY += size) {
                int pixelX = x + glyph->xOffset * size;
                for (int gx = 0; gx < glyph->width; ++gx, pixelX += size, bits <<= 1) {
                    if ((bi++ % 8) == 0)
                        bits = *(bitmap++);
                    if (bits & 0x80) {
                        //if (size == 1)
                            buffer_[map(pixelX, pixelY)] = color;
                        //else
                        //    fill(Rect::XYWH(pixelX, pixelY, size, size), color);
                    }
                }
            }
            return glyph->xAdvance * size;
        }

        Color fg_;
        Color bg_;
        GFXfont const * font_;
        int x_ = 0;
        int y_ = 0;

        Color * buffer_;
        int w_;
        int h_;

    }; // rckid::Canvas

} // namespace rckid