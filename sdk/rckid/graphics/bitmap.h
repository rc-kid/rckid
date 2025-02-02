#pragma once

#include <memory>

#include "../rckid.h"
#include "color.h"
#include "palette.h"
#include "pixel_array.h"
#include "font.h"

namespace rckid {

    template<typename COLOR, typename ALLOCATOR = Heap>
    class Bitmap {
    public:
        static constexpr uint8_t BPP = COLOR::BPP;
        using Color = COLOR;
        using Allocator = ALLOCATOR;
        using PixelArray = rckid::PixelArray<BPP>;

        /** Creates the bitmap using given allocator.
         */
        Bitmap(Coord w, Coord h): pixels_{ALLOCATOR::template alloc<uint8_t>(PixelArray::numBytes(w, h))}, w_{w}, h_{h} {
        }

        /** Frees the bitmap.
         */
        ~Bitmap() {
            ALLOCATOR::free(pixels_);
        }

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
            uint16_t colors[] = {
                0x0000, 
                0x6060,
                0xb0b0,
                0xffff,
            };
            return text(x, y, font, colors);
        }


        //@}

    private:

        Coord putChar(Coord x, Coord y, Font const & font, char c, uint16_t const * colors) {
            return PixelArray::putChar(x, y, w_, h_, font, c, colors, pixels_);
        }

        Writer text(Coord x, Coord y, Font const & font, uint16_t colors[4]) {
            int startX = x;
            return Writer{[=](char c) mutable {
                if (c != '\n') {
                    if (x < w_)
                        x += putChar(x, y, font, c, colors);
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

    /** Bitmap that can render itself. 
     
        Defined as template and specialized based on used color and allocators. See the specializations below for more information.  
     */
    template<typename COLOR, typename ALLOCATOR = Heap>
    class RenderableBitmap;

    /** Renderable bitmap specialization for Full 16bit RGB colors. 
     
        This is rather simple as the entire pixel array of the bitmap can be sent directly to the displayUpdate() method. Initialization simply sets the display to native mode and sets update region to a centered rectangle of the bitmap's size. There is no need to finalize anything. 
      */
    template<typename ALLOCATOR>
    class RenderableBitmap<ColorRGB565, ALLOCATOR> : public Bitmap<ColorRGB565, ALLOCATOR> {
    public:
        using Bitmap<ColorRGB565, ALLOCATOR>::width;
        using Bitmap<ColorRGB565, ALLOCATOR>::height;
        using Bitmap<ColorRGB565, ALLOCATOR>::pixels;
        using Bitmap<ColorRGB565, ALLOCATOR>::numPixels;
        using Bitmap<ColorRGB565, ALLOCATOR>::Bitmap;

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