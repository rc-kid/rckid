#pragma once
#include <type_traits>

#include "../rckid.h"
#include "drawing.h"
#include "font.h"
#include "png.h"

namespace rckid {

    /** Pixel bitmap, templated by the underlying color type. 

        The bitmap manages the pixel data and provides access to it in two different modes - slow per pixel manipulation and faster blitting of regions.      
     */
    template<typename COLOR>
    class Bitmap {
    public:
        using Color = COLOR;
        static constexpr uint8_t BPP = Color::BPP;

        static_assert(BPP == 16 || BPP ==8 || BPP == 4);

        Bitmap() = default;
        Bitmap(Coord width, Coord height): w_{width}, h_{height}, buffer_{allocateBuffer(width, height) } {}

        Bitmap(Bitmap const &) = delete;
        Bitmap(Bitmap && from): w_{from.w_}, h_{from.h_}, buffer_{from.buffer_} {
            from.w_ = 0;
            from.h_ = 0;
            from.buffer_ = nullptr;
        }

        ~Bitmap() {
            delete [] buffer_;
        }

        Coord width() const { return w_; }
        Coord height() const { return h_; }

        uint32_t numPixels() const { return w_ * h_; }

        /** Returns the color buffer. 
         
            This method is only implemented for bitmaps with bit depths allowing zero cost cast between the raw buffer and color types (i.e. 16 and 8 bpp). 
         */
        Color const * buffer() const;

        /** Returns the raw color buffer in the underlying type. 
         */
        const typename Color::RawBufferType rawBuffer() const {
            return reinterpret_cast<const typename Color::RawBufferType>(buffer_);
        }

        /** \name Per-pixel interface 

            A very slow interface that provides per pixel access to the bitmap. On a 16BPP color, filling up the 320x240 screen using the per-pixel interface on RP200 takes around 12ms, which is way too slow for 60fps.  
         */
        //@{

        Color pixelAt(Coord x, Coord y) const { return pixelBufferAt<COLOR>(buffer_, x, y, w_, h_); }

        void setPixelAt(Coord x, Coord y, Color c) { setPixelBufferAt<COLOR>(buffer_, x, y, c, w_, h_); }
        //@}

        /** \name Blitting 
         */
        //@{

        void blit(Point where, Bitmap const & src) { blit(where, src, Rect::WH(src.width(), src.height())); }

        void blit(Point where, Bitmap const & src, Rect srcRect) {
            // default, very slow implementation 
            int dy = where.y;
            for (int y = srcRect.top(), ye = srcRect.bottom(); y != ye; ++y, ++dy) {
                int dx = where.x;
                for (int x = srcRect.left(), xe = srcRect.right(); x != xe; ++x, ++dx)
                    setPixelAt(dx, dy, src.pixelAt(x, y));
            }
        }
        //@}


        /** \name Drawing interface
         */
        //@{

        void fill(Color color) { pixelBufferFill<Color>(buffer_, numPixels(), color); }

        void fill(Color color, Rect rect) {
            // default, very slow implementation
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setPixelAt(x, y, color);
        }

        int putChar(Point where, Font const & font, char c, Color const * colors) {
            if (where.x > width())
                return 0;
            GlyphInfo const & g = font.glyphs[static_cast<uint8_t>((c - 32 >= 0) ? (c - 32) : 0)];
            if (where.x + g.advanceX < 0)
                return g.advanceX;
            uint32_t const * pixels = font.pixels + g.index;
            int ys = where.y + g.y;
            int ye = ys + g.height;
            for (int x = where.x + g.x,xe = where.x + g.x + g.width; x < xe; ++x) {
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

        Writer text(int x, int y, Font const & font, Color const * colors) {
            int startX = x;
            return Writer{[=](char c) mutable {
                if (c != '\n') {
                    x += putChar(Point{x, y}, font, c, colors);
                    if (x < width())
                        return; 
                }
                x = startX;
                y += font.size;
            }};
        }

        Writer text(int x, int y, Font const & font, Color color);

        //@}

        /** \name Image support
         
            Bitmaps can load their contents from PNG and JPG images as part of the SDK. 
         */
        //@{

        void loadImage(PNG && png, Point where = Point::origin()) {
            ASSERT(png.width() <= width());
            ASSERT(png.height() <= height());
            ASSERT(buffer_ != nullptr);
            png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
                for (int i = 0; i < lineWidth; ++i)
                    setPixelAt(i + where.x, lineNum + where.y, line[i]);
            });
        }

        static Bitmap fromImage(PNG && png) {
            Bitmap result{png.width(), png.height()};
            result.loadImage(std::move(png));
            return result;
        }
        //@}

    private:

        constexpr uint8_t * allocateBuffer(int w, int h) {
            if (w == 0 || h == 0)
                return nullptr;
            return new uint8_t[pixelBufferSize<COLOR>(w, h)];
        } 

        constexpr __force_inline size_t map(Coord x, Coord y) const { return pixelBufferOffset(x, y, w_, h_); }

        Coord w_ = 0;
        Coord h_ = 0;

        uint8_t * buffer_ = nullptr;




    }; // rckid::Bitmap


    template<>
    inline ColorRGB const * Bitmap<ColorRGB>::buffer() const {
        return reinterpret_cast<ColorRGB const *>(buffer_);
    }

    template<>
    inline Color256 const * Bitmap<Color256>::buffer() const {
        return reinterpret_cast<Color256 const *>(buffer_);
    }

    template<>
    inline Writer Bitmap<ColorRGB>::text(int x, int y, Font const & font, ColorRGB color) {
        Color colors[] = {
            color.withAlpha(0), 
            color.withAlpha(85), 
            color.withAlpha(170), 
            color.withAlpha(255), 
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

} // namespace rckid