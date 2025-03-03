#pragma once

#include <memory>

#include "../rckid.h"
#include "../memory.h"
#include "palette.h"
#include "pixel_array.h"
#include "font.h"
#include "png.h"

namespace rckid {

    template<uint32_t BPP>
    class Bitmap {
    public:
        using PixelArray = rckid::PixelArray<BPP>;
        using Pixel = typename PixelArray::Pixel;

        /** Default constructor that creates an empty bitmap with no pixel buffer
         */
        Bitmap():
            pixels_{nullptr}, w_{0}, h_{0} {
        }

        /** Creates the bitmap using given allocator.
         */
        Bitmap(Coord w, Coord h, Allocator & a = Heap::allocator()): pixels_{a.alloc<Pixel>(w * h)}, w_{w}, h_{h} {
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

        Bitmap & operator = (Bitmap && other) {
            Heap::tryFree(pixels_);
            pixels_ = other.pixels_;
            w_ = other.w_;
            h_ = other.h_;
            other.pixels_ = nullptr;
            other.w_ = 0;
            other.h_ = 0;
            return *this;
        }

        /** \name Per-pixel interface 

            A very slow interface that provides per pixel access to the bitmap.   
         */
        //@{

        Pixel at(Coord x, Coord y) const { return PixelArray::get(x, y, w_, h_, pixels_); }

        void setAt(Coord x, Coord y, Pixel c) { PixelArray::set(x, y, w_, h_, c, pixels_); }

        Pixel const * pixels() const { return pixels_; }

        /** Returns the pixels of the specified column. Expects the column to be in valid range. 
         */
        Pixel * columnPixels(Coord column) {
            return pixels_ + PixelArray::offset(column, 0, w_, h_);
        }

        //@}

        /** \name Blitting. 
         */
        //@{
        void blit(Point where, Bitmap<BPP> const & src) { blit(where, src, Rect::WH(src.width(), src.height())); }

        void blit(Point where, Bitmap<BPP> const & src, Rect srcRect) {
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
        void fill(Pixel color) { fill(color, Rect::WH(w_, h_)); }

        void fill(Pixel color, Rect rect) {
            // default, very slow implementation
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setAt(x, y, color);
        }

        Writer text(Coord x, Coord y, Font const & font, std::array<Pixel, 4> colors) {
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

        //@}

        /** Renders the given column of the bitmap. 
         
            This method allows for easy use of bitmaps in the UI column-wise rendering pipeline. Note that since all rendering only happens in 16 bpp pixel arrays, the method is very slow for 8bpp bitmaps as each pixel has to be translated to 16bpp, whereas 16 to 16 bpp render is a simple memcopy.
         */
        void renderColumn(Coord column, rckid::PixelArray<16>::Pixel * buffer, Coord starty, Coord numPixels) {
            ASSERT(column >= 0 && column < w_);
            ASSERT(starty >= 0 && starty + numPixels <= h_);
            // default, very slow implementation
            for (int y = starty, ye = starty + numPixels; y < ye; ++y)
                buffer[y - starty] = ColorRGB::fromRaw(at(column, y)).raw16();
        }

    private:

        Coord putChar(Coord x, Coord y, Font const & font, char c, Pixel const * colors) {
            return PixelArray::putChar(x, y, w_, h_, font, c, colors, pixels_);
        }

        Pixel * pixels_;
        Coord w_;
        Coord h_;
    }; // rckid::Bitmap

    template<>
    inline void Bitmap<16>::loadImage(PNG && decoder, int x, int y) {
        decoder.decode16([this, x, y](Pixel * line, int lineNum, int lineWidth) {
            for (int i = 0; i < lineWidth; ++i)
                setAt(i + x, lineNum + y, line[i]);
        });
    }

    /** Bitmap that can render itself. 
     
        Defined as template and specialized based on used color and allocators. See the specializations below for more information.  
     */
    template<uint32_t BPP>
    class RenderableBitmap {

    };

    /** Renderable bitmap specialization for Full 16bit RGB colors. 
     
        This is rather simple as the entire pixel array of the bitmap can be sent directly to the displayUpdate() method. Initialization simply sets the display to native mode and sets update region to a centered rectangle of the bitmap's size. There is no need to finalize anything. 
      */
    template<>
    class RenderableBitmap<16> : public Bitmap<16> {
    public:
        using Bitmap<16>::width;
        using Bitmap<16>::height;
        using Bitmap<16>::pixels;
        using Bitmap<16>::numPixels;
        using Bitmap<16>::Bitmap;

        RenderableBitmap(Allocator & a = Heap::allocator()):
            Bitmap{RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT, a} {
        }

        void initialize() {
            initialize(Rect::Centered(width(), height(), RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT));
        }

        void initialize(Rect updateRect) {
            displaySetRefreshDirection(DisplayRefreshDirection::ColumnFirst);
            displaySetUpdateRegion(updateRect);
        }

        void finalize() {
            // noting in the finalizer
        }

        /** Renderer's app API, for bitmap there is nothing we need to do.
         */
        void update() {
        }

        void render() {
            displayWaitVSync();
            displayUpdate(pixels(), numPixels());
        }

    };
    
}