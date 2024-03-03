#pragma once

#include "rckid/writer.h"

#include "gfx.h"
#include "color.h"
#include "primitives.h"
#include "png.h"
#include "bitmap.h"
#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    template<typename COLOR>
    class Canvas : public Bitmap<COLOR> {
    public:

        using typename Bitmap<COLOR>::Color;
        using Bitmap<COLOR>::w_;
        using Bitmap<COLOR>::h_;
        using Bitmap<COLOR>::setPixelAt;
        using Bitmap<COLOR>::pixelAt;

        Canvas(int width, int height) : Bitmap<COLOR>{width, height} {}

        Canvas(int width, int height, uint32_t * buffer) : Bitmap<COLOR>{width, height, buffer} {}

        Color bg() const { return bg_; }
        Color fg() const { return fg_; }

        void setFg(Color c) { fg_ = c; }
        void setBg(Color c) { bg_ = c; }

        GFXfont const & font() const { return * font_; }
        void setFont(GFXfont const & font) { font_ = &font; }

        Writer text(int x, int y) {
            return Writer{[this, x, y] (char c) mutable {
                if (c != '\n')
                    x += drawGlyph(x, y, c, fg_, font_, 1);
            }};
        }

        Writer text(Point p) { return text(p.x(), p.y()); }

        Writer textMultiline(int x, int y) {
            int startX = x;
            return Writer{[this, x, y, startX] (char c) mutable {
                if (c != '\n') {
                    x += drawGlyph(x, y, c, fg_, font_, 1);
                } else {
                    x = startX;
                    y += font().yAdvance;
                }
            }};
        }

        /// TODO: Move this to font so that we can share this between different color bpps
        int textWidth(std::string const & text) const { return textWidth(text.c_str()); }

        /// TODO: Move this to font so that we can share this between different color bpps
        int textWidth(char const * text) const {
            int width = 0;
            while (*text != 0) {
                width += font_->glyph[(*text++ - font_->first)].xAdvance;
            }
            return width;
        }

        using Bitmap<COLOR>::fill;

        /** Fills entire screen with the selected background color. 
         */
        void fill() { fill(Rect::WH(w_, h_)); }

        /** Fills the given rectangle with the selected background color. 
         */
        void fill(Rect rect) { fill(rect, bg_); }


    private:

        int drawGlyph(int x, int y, char c, Color color, GFXfont const * f, int size) {
            GFXglyph * glyph = f->glyph + (c - f->first);
            uint8_t const * bitmap = f->bitmap + glyph->bitmapOffset;
            int pixelY = y + glyph->yOffset * size + f->yAdvance;
            int bi = 0;
            uint8_t bits = *bitmap;
            for (int gy = 0; gy < glyph->height; ++gy, pixelY += size) {
                int pixelX = x + glyph->xOffset * size;
                for (int gx = 0; gx < glyph->width; ++gx, pixelX += size, bits <<= 1) {
                    if ((bi++ % 8) == 0)
                        bits = *(bitmap++);
                    if (bits & 0x80) {
                        setPixelAt(pixelX, pixelY, color);
                        //if (size == 1)
                        //    buffer_[map(pixelX, pixelY)] = color;
                        //else
                        //    fill(Rect::XYWH(pixelX, pixelY, size, size), color);
                    } 
                }
            }
            return glyph->xAdvance * size;
        }

        Color fg_ = Color::White();
        Color bg_ = Color::Black();
        GFXfont const * font_ = & Iosevka_Mono6pt7b;

    }; // rckid::Canvas

    template<>
    inline void Canvas<ColorRGB>::fill() {
        uint32_t x = bg_.rawValue16() << 16 | bg_.rawValue16();
        uint32_t * b = reinterpret_cast<uint32_t*>(buffer_);
        for (unsigned i = 0, e = numPixels() / 2; i < e; ++i)
            b[i] = x;
    }

    template<>
    inline void Canvas<Color256>::fill() {
        memset(buffer_, bg_.index(), 320 * 240);
    }


} // namespace rckid