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

        /** Fills entire screen with the selected background color. 
         */
        void fill() {
            fill(Rect::WH(w_, h_));
        }

        /** Fills the given rectangle with the selected background color. 
         */
        void fill(Rect rect) {
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setPixelAt(x, y, bg_);
        }


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
            //buffer_[i] = bg_;
            b[i] = x;

    }

/*

    class Canvas : public Bitmap<ColorRGB> {
    public:

        Canvas(int width, int height): Bitmap{width, height} {}

        ColorRGB bg() const { return bg_; }
        ColorRGB fg() const { return fg_; }

        void setFg(ColorRGB c) { fg_ = c; }
        void setBg(ColorRGB c) { bg_ = c; }

        GFXfont const & font() const { return * font_; }
        void setFont(GFXfont const & font) { font_ = &font; }

        Writer text(int x, int y) {
            x_ = x;
            y_ = y;
            return text();
        }

        Writer text(Point p) { return text(p.x(), p.y()); }

        Writer text() {
            return Writer([this](char c){
                if (c != '\n')
                    x_ += drawGlyph(x_, y_, c, fg_, font_, 1);
            });
        }

        int textWidth(std::string const & text) const { return textWidth(text.c_str()); }

        int textWidth(char const * text) const {
            int width = 0;
            while (*text != 0) {
                width += font_->glyph[(*text++ - font_->first)].xAdvance;
            }
            return width;
        }

        / ** Fills entire screen with the selected background color. 
         * /
        void fill() {
            uint32_t x = bg_.rawValue16() << 16 | bg_.rawValue16();
            uint32_t * b = reinterpret_cast<uint32_t*>(buffer_);
            for (unsigned i = 0, e = numPixels() / 2; i < e; ++i)
                //buffer_[i] = bg_;
                b[i] = x;
        }

        / ** Fills the given rectangle with the selected background color. 
         * /
        void fill(Rect rect) {
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setPixelAt(x, y, bg_);
        }

    private:

        int drawGlyph(int x, int y, char c, ColorRGB color, GFXfont const * f, int size) {
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

        ColorRGB fg_;
        ColorRGB bg_;
        GFXfont const * font_ = & Iosevka_Mono6pt7b;
        int x_ = 0;
        int y_ = 0;

    }; // rckid::Canvas


    class Image {
    public:
        Image(int width, int height):
            buffer_{ width * height == 0 ? nullptr : new ColorRGB[ width * height] },
            w_{width},
            h_{height} {
        }

        ~Image() { delete [] buffer_; }

        int width() const { return w_; }
        int height() const { return h_; }

        void resize(int width, int height) {
            if (w_ != width || h_ != height) {
                delete [] buffer_;
                buffer_ = new ColorRGB[width * height];
                w_ = width;
                h_ = height;
            }
        }

        void swapWith(Image & other) {
            std::swap(buffer_, other.buffer_);
            std::swap(w_, other.w_);
            std::swap(h_, other.h_);
        }

        / ** Loads the given png to the canvas. The canvas is resized according to the image's dimensions. 
         * /
        void loadImage(PNG & png) {
            resize(png.width(), png.height());
            png.decode([this](ColorRGB * line, int lineNum, int lineWidth){
                for (int i = 0; i < lineWidth; ++i)
                    setPixel(i, lineNum, line[i]);
            });
        }

        void draw(Image const & from, int x, int y) { 
            draw(from, x, y, Rect::WH(from.width(), from.height()));
        }

        void draw(Image const & from, Point where) {
            draw(from, where.x(), where.y(), Rect::WH(from.width(), from.height()));
        }

        void draw(Image const & from, Point where, Rect fromRect) {
            draw(from, where.x(),  where.y(), fromRect);
        }

        void draw(Image const & from, int x, int y, Rect fromRect) {
            for (int xx = 0, xe = fromRect.width(); xx != xe; ++xx)
                for (int yy = 0, ye = fromRect.height(); yy != ye; ++yy) 
                    setPixel(x + xx, y + yy, from.buffer_[from.map(fromRect.left() + xx, fromRect.top() + yy)]);
        }

        void setPixel(int x, int y, ColorRGB c) { 
            if (x > 0 && x < w_ && y > 0 && y < h_)
                buffer_[map(x, y)] = c; 
        }

        uint16_t const * rawPixels() const { 
            static_assert(sizeof(ColorRGB) == sizeof(uint16_t));
            return reinterpret_cast<uint16_t const *>(buffer_); 
        }

        size_t rawPixelsCount() const { return w_ * h_; }

    protected:

        constexpr size_t map(int x, int y) const __attribute__((always_inline)){ 
            return (w_ - x - 1) * h_ + y; 
        }

        ColorRGB * buffer_;
        int w_;
        int h_;

    }; // rckid::Image



    / ** Image with fixed palette colors. 
     * /
    class Image256 {
    public:

    protected:

        uint8_t buffer_;
        int w_;
        int h_;


    }; // Image256

*/


} // namespace rckid