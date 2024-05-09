#pragma once

#include "rckid/rckid.h"
#include "primitives.h"
#include "png.h"
#include "font.h"

namespace rckid {

    /** Pixel bitmap, templated by the underlying color type. 
     
        The bitmap supportes heap or VRAM allocation and manages the underlying array of pixels and its size. Furthermore, the bitmap supports drawing itself on other bitmaps and other primitive drawing operations. 
     */
    template<typename COLOR>
    class Bitmap {
    public:
        using Color = COLOR;

        Bitmap() = default;
        Bitmap(int w, int h): 
            w_{w}, 
            h_{h},
            buffer_{allocateBuffer(w_, h_)} {
        }

        Bitmap(Bitmap && from):
            w_{from.w_}, 
            h_{from.h_},
            buffer_{from.buffer_} {
                from.w_ = 0;
                from.h_ = 0;
                from.buffer_ = nullptr;
        }

        ~Bitmap() {
            delete buffer_;
        }

        Bitmap & operator = (Bitmap && other) {
            delete buffer_;
            w_ = other.w_;
            h_ = other.h_;
            buffer_ = other.buffer_;
            other.buffer_ = nullptr;
            return *this;
        }

        void resize(int w, int h) {
            delete buffer_;
            w_ = w;
            h_ = h;
            buffer_ = allocateBuffer(w_, h_);
        }

        int width() const { return w_; }
        int height() const { return h_; }
        size_t numPixels() const { return w_ * h_; }

        Color * rawBuffer() { return buffer_; }


        /** \name Per-pixel interface 

            A very slow interface that provides per pixel access to the bitmap. On a 16BPP color, filling up the 320x240 screen using the per-pixel interface on RP200 takes around 12ms, which is way too slow for 60fps.  
         */
        //@{
        Color pixelAt(int x, int y) const {
            return buffer_[map(x, y)];
        }

        template<typename SRC_COLOR>
        void setPixelAt(int x, int y, SRC_COLOR c) {
            if (x >= 0 && x < width() && y >= 0 && y < height())
                buffer_[map(x, y)] = c;
        }
        //@}

        /** \name Drawing
         */
        //@{

        void draw(Point where, Bitmap const & src) { draw(where, src, Rect::WH(src.width(), src.height())); }

        void draw(Point where, Bitmap const & src, Rect srcRect) {
            // default, very slow implementation 
            int dy = where.y();
            for (int y = srcRect.top(), ye = srcRect.bottom(); y != ye; ++y, ++dy) {
                int dx = where.x();
                for (int x = srcRect.left(), xe = srcRect.right(); x != xe; ++x, ++dx)
                    setPixelAt(dx, dy, src.pixelAt(x, y));
            }
        }
        //@}

        /** \name Image support
         
            Bitmaps can load their contents from PNG and JPG images as part of the SDK. 
         */
        //@{

        void loadImage(uint8_t const * buffer, size_t numBytes) {
            loadImage(PNG::fromBuffer(buffer, numBytes));
        }
        
        void loadImage(PNG && png, Point where = Point::origin()) {
            ASSERT(png.width() <= width());
            ASSERT(png.height() <= height());
            ASSERT(buffer_ != nullptr);
            png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
                for (int i = 0; i < lineWidth; ++i)
                    setPixelAt(i + where.x(), lineNum + where.y(), line[i]);
            });
        }
        //@}

        /** Fills the portion of the bitmap with given color. 
         
         */
        void fill(Rect rect, Color color) {
            // default, very slow implementation
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setPixelAt(x, y, color);
        }

        void fill(Color color) { fill(Rect::WH(width(), height()), color); }


        /** Text routines. 
         */

        //@{

        int putChar(Point where, Font const & font, char c, Color * colors) {
            GlyphInfo const & g = font.glyphs[static_cast<uint8_t>((c - 32 >= 0) ? (c - 32) : 0)];
            uint32_t const * pixels = font.pixels + g.index;
            unsigned a;
            unsigned col;
            int ys = where.y() + g.y;
            int ye = ys + g.height;
            for (int x = where.x() + g.x,xe = where.x() + g.x + g.width; x < xe; ++x) {
                uint32_t col;
                uint32_t bits = 0;
                for (int y = ys; y != ye; ++y) {
                    if (bits == 0) {
                        bits = 32;
                        col = *pixels++;
                    }
                    unsigned a = (col >> 30) & 0x3;
                    if (a != 0)
                        setPixelAt(x, y, colors[a]);
                    col = col << 2;
                    bits -= 2;
                }
            }
            return g.advanceX;
        }

        int putChar(Point where, Font const & font, char c, Color color) {
            Color colors[] = {
                Color::Black(), 
                color.withAlpha(1), 
                color.withAlpha(2), 
                color.withAlpha(3), 
            };
            return putChar(where, font, c, colors);
        }

        Writer text(int x, int y, Font const & font, Color color) {
            Color colors[] = {
                Color::Black(), 
                color.withAlpha(1), 
                color.withAlpha(2), 
                color.withAlpha(3), 
            };
            return Writer{[this, x, y, font, colors](char c) mutable {
                if (c != '\n')
                    x += putChar(Point{x, y}, font, c, colors);
            }};
        }

        Writer textMultiline(int x, int y, Font const & font, Color color) {
            Color colors[] = {
                Color::Black(), 
                color.withAlpha(1), 
                color.withAlpha(2), 
                color.withAlpha(3), 
            };
            int startX = x;
            return Writer{[this, x, startX, y, font, colors](char c) mutable {
                if (c != '\n') {
                    x += putChar(Point{x, y}, font, c, colors);
                    if (x < width())
                        return; 
                }
                x = startX;
                y += font.size;
            }};
        }
        //@}


    protected:

        constexpr Color * allocateBuffer(int w, int h) {
            if (w ==0 || h == 0)
                return nullptr;
            return new Color[w * h];
        } 

        constexpr __force_inline size_t map(int x, int y) const { return map(x, y, w_, h_); }

        static __force_inline size_t map(int x, int y, int w, int h) { 
            return (w - x - 1) * h + y; 
        }

        int w_ = 0;
        int h_ = 0;
        Color * buffer_ = nullptr;

    }; // rckid::Bitmap

    template<>
    inline void Bitmap<ColorRGB>::fill(Color color) {
        uint32_t c = (static_cast<uint32_t>(color.rawValue16()) << 16) | color.rawValue16();
        rckid_mem_fill_32x8(reinterpret_cast<uint32_t*>(buffer_), w_ * h_ / 2, c);        
        /*
        int i = 0, e = w_ * h_ / 2;
        for ( ; i <= e - 16; ) {
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
            buffer_[i++] = c;
        } 
        for ( ; i < e; )
            buffer_[i++] = c;
            */
    }

    template<>
    inline void Bitmap<ColorRGB>::fill(Rect rect, Color color) {
        Rect r = Rect::WH(w_, h_) && rect;
        if (r.right() == w_ && r.height() == h_ && r.top() == 0)
            return fill(color);
        // fill the first row if not even
        int y = r.top(), ye = r.bottom();
        if (y % 2 != 0) {
            for (int x = r.left(), xe = r.right(); x < xe; ++x)
                buffer_[map(x, y)] = color;
            ++y;
        }
        // now do the manually unrolled loop with two rows per color fixed
        uint32_t * buffer = reinterpret_cast<uint32_t*>(buffer_);
        uint32_t c = (static_cast<uint32_t>(color.rawValue16()) << 16) | color.rawValue16();
        for (; y <= ye - 16; y += 16) {
            for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                uint32_t * b = buffer + map(x, y) / 2;
                b[0] = c;
                b[1] = c;
                b[2] = c;
                b[3] = c;
                b[4] = c;
                b[5] = c;
                b[6] = c;
                b[7] = c;
            }
        }

        for (; y <= ye - 8; y += 8) {
            for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                uint32_t * b = buffer + map(x, y) / 2;
                b[0] = c;
                b[1] = c;
                b[2] = c;
                b[3] = c;
            }
        }
        // finish per two rows
        for (; y <= ye - 2; y += 2) {
            for (int x = r.left(), xe = r.right(); x < xe; ++x) {
                uint32_t * b = buffer + map(x, y) / 2;
                b[0] = c;
            }
        }
        // and the optional single row access at the end
        for (; y < ye; ++y) {
            for (int x = r.left(), xe = r.right(); x < xe; ++x)
                buffer_[map(x, y)] = color;
        }
    }

} // namespace rckid