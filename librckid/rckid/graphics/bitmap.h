#pragma once

#include "../rckid.h"
#include "primitives.h"

namespace rckid {

    /** Pixel bitmap, templated by the underlying color type. 
     
        The bitmap supportes heap or VRAM allocation and manages the underlying array of pixels and its size. Furthermore, the bitmap supports drawing itself on other bitmaps and other primitive drawing operations. 
     */
    template<typename COLOR>
    class Bitmap {
    public:
        using Color = COLOR;

        Bitmap() = default;
        Bitmap(int w, int h, MemArea where = MemArea::Heap): w_{w}, h_{h} {
            allocate(where);
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
            deallocate();
        }

        Bitmap & operator = (Bitmap && other) {
            deallocate();
            w_ = other.w_;
            h_ = other.h_;
            buffer_ = other.buffer_;
            other.buffer_ = nullptr;
            return *this;
        }

        void allocate(MemArea where = MemArea::Heap) {
            if (buffer_ != nullptr)
                return;
            ASSERT((w_ * Color::BPP) % 32 == 0);
            ASSERT((h_ * Color::BPP) % 32 == 0);
            if (w_ != 0 && h_ != 0) {
                size_t numBytes = Color::BPP * w_ * h_ / 8;
                buffer_ = static_cast<uint32_t *>(rckid::allocate(numBytes, where));
            }
        }

        void deallocate() {
            rckid::deallocate(buffer_);
        }

        void resize(int w, int h, MemArea where = MemArea::Heap) {
            deallocate();
            w_ = w;
            h_ = h;
            allocate(where);
        }

        int width() const { return w_; }
        int height() const { return h_; }
        size_t numPixels() const { return w_ * h_; }

        Color * rawBuffer() { return reinterpret_cast<Color*>(buffer_); }


        /** \name Per-pixel interface 

            A very slow interface that provides per pixel access to the bitmap. On a 16BPP color, filling up the 320x240 screen using the per-pixel interface on RP200 takes around 12ms, which is way too slow for 60fps.  
         */
        //@{
        Color pixelAt(int x, int y) const {
            Color const * buf = reinterpret_cast<Color const *>(buffer_);
            //LOG("rd " << x << ", " << y << " [" << map(x, y) << "]: " << (int)*(reinterpret_cast<uint8_t const *>(buf + map(x, y))));
            return buf[map(x, y)];
            //return reinterpret_cast<Color const *>(buffer_)[map(x, y)];
        }

        template<typename SRC_COLOR>
        void setPixelAt(int x, int y, SRC_COLOR c) {
            if (x >= 0 && x < width() && y >= 0 && y < height()) {
                Color * buf = reinterpret_cast<Color*>(buffer_);
                buf[map(x, y)] = c;
                //LOG("wr " << x << "," << y << "[" << map(x, y) << "]: " << (int)*reinterpret_cast<uint8_t*>(buf + map(x, y)));
                pixelAt(x, y);
            }
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

    protected:

        constexpr __force_inline size_t map(int x, int y) const { return map(x, y, w_, h_); }

        static __force_inline size_t map(int x, int y, int w, int h) { 
            size_t result = (w - x - 1) * h + y;
            if (result >= 25600)
                LOG("oopsies");
            return (w - x - 1) * h + y; 
            }

        int w_ = 0;
        int h_ = 0;
        uint32_t * buffer_ = nullptr;

    }; // rckid::Bitmap

} // namespace rckid