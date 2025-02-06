#pragma once

#include <memory>

#include "../rckid.h"
#include "../memory.h"
#include "color.h"
#include "palette.h"
#include "pixel_array.h"
#include "font.h"
#include "png.h"

namespace rckid {

    template<typename COLOR>
    class Bitmap {
    public:
        static constexpr uint8_t BPP = COLOR::BPP;
        using Color = COLOR;
        using PixelArray = rckid::PixelArray<BPP>;

        /** Creates the bitmap using given allocator.
         */
        Bitmap(Coord w, Coord h, Allocator & a = Heap::allocator()): pixels_{a.alloc<uint8_t>(PixelArray::numBytes(w, h))}, w_{w}, h_{h} {
        }

        Bitmap(Bitmap const &) = delete;

        Bitmap(Bitmap && other): pixels_{other.pixels_}, w_{other.w_}, h_{other.h_} {
            other.pixels_ = nullptr;
            other.w_ = 0;
            other.h_ = 0;
        }

        // TODO add bitmap constructor with decoder and palette that will be specialized for nonrgb bitmaps
        Bitmap(PNG && decoder, Allocator & a = Heap::allocator()):
            Bitmap{decoder.width(), decoder.height(), a} {
            loadImage(std::move(decoder), 0, 0);
        }

        /** Frees the bitmap.
         */
        ~Bitmap() { Heap::tryFree(pixels_); };

        void loadImage(PNG && decoder, int x, int y);
        void loadImage(PNG && decoder) { loadImage(std::move(decoder), 0, 0); }

        Coord width() const { return w_; }
        Coord height() const { return h_; }

        uint32_t numPixels() const { return w_ * h_; };
        uint32_t numBytes() const { return PixelArray::size(w_, h_); }

        /** \name Per-pixel interface 

            A very slow interface that provides per pixel access to the bitmap.   
         */
        //@{

        Color at(Coord x, Coord y) const { return Color::fromRaw(PixelArray::get(x, y, w_, h_, pixels_)); }

        void setAt(Coord x, Coord y, Color c) { PixelArray::set(x, y, w_, h_, c.raw(), pixels_); }

        uint8_t const * pixels() const { return pixels_; }

        //@}

        /** \name Blitting. 
         */
        //@{
        void blit(Point where, Bitmap<COLOR> const & src) { blit(where, src, Rect::WH(src.width(), src.height())); }

        void blit(Point where, Bitmap<COLOR> const & src, Rect srcRect) {
            // default, very slow implementation 
            int dy = where.y;
            for (int y = srcRect.top(), ye = srcRect.bottom(); y != ye; ++y, ++dy) {
                int dx = where.x;
                for (int x = srcRect.left(), xe = srcRect.right(); x != xe; ++x, ++dx)
                    setAt(dx, dy, src.at(x, y));
            }
        }
        //@}

        /** \name Drawing
         */
        //@{
        void fill(Color color) { fill(color, Rect::WH(w_, h_)); }

        void fill(Color color, Rect rect) {
            // default, very slow implementation
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setAt(x, y, color);
        }

        Writer text(Coord x, Coord y, Font const & font, Color color) {
            return text(x, y, font, Font::colorToArray(color));
        }

        //@}

    private:

        Coord putChar(Coord x, Coord y, Font const & font, char c, uint16_t const * colors) {
            return PixelArray::putChar(x, y, w_, h_, font, c, colors, pixels_);
        }

        Writer text(Coord x, Coord y, Font const & font, std::array<uint16_t, 4> colors) {
            int startX = x;
            return Writer{[=](char c) mutable {
                if (c != '\n') {
                    if (x < w_)
                        x += putChar(x, y, font, c, colors.data());
                } else {
                    x = startX;
                    y += font.size;
                }
            }};
        }

        uint8_t * pixels_;
        Coord w_;
        Coord h_;
    }; // rckid::Bitmap

    template<>
    inline void Bitmap<ColorRGB565>::loadImage(PNG && decoder, int x, int y) {
        decoder.decode([this, x, y](ColorRGB * line, int lineNum, int lineWidth) {
            for (int i = 0; i < lineWidth; ++i)
                setAt(i + x, lineNum + y, line[i]);
        });
    }

    /** Bitmap that can render itself. 
     
        Defined as template and specialized based on used color and allocators. See the specializations below for more information.  
     */
    template<typename COLOR>
    class RenderableBitmap;

    /** Renderable bitmap specialization for Full 16bit RGB colors. 
     
        This is rather simple as the entire pixel array of the bitmap can be sent directly to the displayUpdate() method. Initialization simply sets the display to native mode and sets update region to a centered rectangle of the bitmap's size. There is no need to finalize anything. 
      */
    template<>
    class RenderableBitmap<ColorRGB565> : public Bitmap<ColorRGB565> {
    public:
        using Bitmap<ColorRGB565>::width;
        using Bitmap<ColorRGB565>::height;
        using Bitmap<ColorRGB565>::pixels;
        using Bitmap<ColorRGB565>::numPixels;
        using Bitmap<ColorRGB565>::Bitmap;

        void initialize() {
            initialize(Rect::Centered(width(), height(), 320, 240), DisplayResolution::Normal);
        }

        void initialize(Rect updateRect, DisplayResolution resolution) {
            displaySetResolution(resolution);
            displaySetRefreshDirection(DisplayRefreshDirection::Native);
            displaySetUpdateRegion(updateRect);
        }

        void finalize() {
            // noting in the finalizer
        }

        void render() {
            displayWaitVSync();
            displayUpdate(reinterpret_cast<ColorRGB565 const *>(pixels()), numPixels());
        }

    };


    
}