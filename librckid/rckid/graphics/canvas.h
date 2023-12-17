#pragma once

#include "rckid/writer.h"

#include "gfx.h"
#include "color.h"
#include "primitives.h"

namespace rckid {

    template<typename PIXEL_FORMAT = ColorRGB>
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

        int width() const { return w_; }
        int height() const { return h_; }

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
                if (c != '\n')
                    x_ += drawGlyph(x_, y_, c, fg_, font_, 1);
                if (x_ >= w_ || c == '\n') {
                    x_ = 0;
                    y_ += font_->yAdvance;
                }
            });
        }

        /** Fills entire screen with the selected background color. 
         */
        void fill() {
            Color c[] = { bg_, bg_ };
            uint32_t x = *(reinterpret_cast<uint32_t*>(& c));
            uint32_t * b = reinterpret_cast<uint32_t*>(buffer_);
            for (unsigned i = 0, e = rawPixelsCount() / 2; i < e; ++i)
                //buffer_[i] = bg_;
                b[i] = x;
        }

        /** Fills the given rectangle with the selected background color. 
         */
        void fill(Rect rect) {
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    buffer_[map(x, y)] = bg_;
        }

        void draw(Canvas const & from, int x, int y) { 
            draw(from, x, y, Rect::WH(from.width(), from.height()));
        }

        void draw(Canvas const & from, Point where) {
            draw(from, where.x(), where.y(), Rect::WH(from.width(), from.height()));
        }

        void draw(Canvas const & from, Point where, Rect fromRect) {
            draw(from, where.x(),  where.y(), fromRect);
        }

        void draw(Canvas const & from, int x, int y, Rect fromRect) {
            for (int xx = 0, xe = fromRect.width(); xx != xe; ++xx)
                for (int yy = 0, ye = fromRect.height(); yy != ye; ++yy)
                    buffer_[map(x + xx, y + yy)] = from.buffer_[from.map(fromRect.left() + xx, fromRect.top() + yy)];
        }

        uint16_t const * rawPixels() const { 
            static_assert(sizeof(Color) == sizeof(uint16_t));
            return reinterpret_cast<uint16_t const *>(buffer_); 
        }

        size_t rawPixelsCount() const { return w_ * h_; }

    private:

        constexpr size_t map(int x, int y) const __attribute__((always_inline)){ 
            return (w_ - x - 1) * h_ + y; 
        }

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
        GFXfont const * font_ = nullptr;
        int x_ = 0;
        int y_ = 0;

        Color * buffer_;
        int w_;
        int h_;

    }; // rckid::Canvas

} // namespace rckid