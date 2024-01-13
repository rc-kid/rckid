#pragma once

#include "../rckid.h"
#include "primitives.h"

namespace rckid {

    template<typename COLOR>
    class Bitmap {
    public:
        using Color = COLOR;

        Bitmap(int w, int h) {
            allocateBuffer(w, h);
        }

        Bitmap(Bitmap const &) = delete;

        Bitmap(Bitmap && other):
            buffer_{other.buffer_},
            w_{other.w_}, 
            h_{other.h_} {
                other.w_ = 0; other.h_ = 0; other.buffer_ = nullptr;
        }

        Bitmap & operator = (Bitmap && other) {
            delete [] buffer_;
            w_ = other.w_;
            h_ = other.h_;
            buffer_ = other.buffer_;
            other.w_ = 0; other.h_ = 0; other.buffer_ = nullptr;
            return *this;
        }

        ~Bitmap() { delete [] buffer_; }

        int width() const { return w_; }
        int height() const { return h_; }

        void resize(int width, int height) {
            if (width != w_ || height != h_) {
                delete [] buffer_;
                allocateBuffer(width, height);
            }
        }

        size_t numPixels() const { return w_ * h_; }

        Color pixelAt(int x, int y) const {
            switch (Color::BPP) {
                // getting the right pixel is simple as we only need to cast the buffer to the underlying type and then map the coordinates                
                case 16:
                case 8:
                    return reinterpret_cast<Color const *>(buffer_)[map(x, y)];
                case 4:
                case 2:
                case 1:
                    UNIMPLEMENTED;
            }
        }

        void setPixelAt(int x, int y, Color c) {
            // don't change the pixel if we are outside of the boundaries
            if (x < 0 || x >= w_ || y < 0 || y >= h_)
                return;
            switch (Color::BPP) {
                // setting right pixel in one and 2 byte version is simple, just map the system accordingly. 
                case 16:
                case 8:
                    reinterpret_cast<Color*>(buffer_)[map(x, y)] = c;
                    break;
                case 4:
                case 2:
                case 1:
                    UNIMPLEMENTED;
            }
        }

        template<typename T>
        void draw(Bitmap<T> const & from, int x, int y) {
            draw<T>(from.buffer_, from.w_, from.h_, Point{x, y}, Rect::WH(from.w_, from.h_));
        }

        template<typename T>
        void draw(Bitmap<T> const & from, Point where) {
            draw<T>(from.buffer_, from.w_, from.h_, where, Rect::WH(from.w_, from.h_));
        }

        template<typename T>
        void draw(Bitmap<T> const & from, int x, int y, Rect fromRect) {
            draw<T>(from.buffer_, from.w_, from.h_, Point{x, y}, fromRect);
        }

        template<typename T>
        void draw(Bitmap<T> const & from, Point where, Rect fromRect) {
            draw<T>(from.buffer_, from.w_, from.h_, where, fromRect);
        }

        void loadImage(PNG && img);

    protected:

        template<typename T>
        friend class Bitmap;

        static __force_inline size_t map(int x, int y, int w, int h) { return (w - x - 1) * h + y; }

        constexpr __force_inline size_t map(int x, int y) const { return (w_ - x - 1) * h_ + y; }

        void allocateBuffer(int w, int h) {
            ASSERT(w % 4 == 0);
            ASSERT(h % 4 == 0);
            buffer_ = (w * h == 0) ? nullptr : new uint32_t[w * h * Color::BPP / 32];
            w_ = w;
            h_ = h;
        }

        /** Internal draw function that draws a rectangle from given bitmap buffer at the predefined top-left coordinate in our canvas.
         */
        template<typename T>
        void draw(uint32_t const * otherBuffer, int otherWidth, int otherHeight, Point where, Rect fromRect); 

        uint32_t * buffer_;
        int w_;
        int h_;
    }; 

    template<>
    inline void Bitmap<ColorRGB>::loadImage(PNG && png) {
        resize(png.width(), png.height());
        png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
            for (int i = 0; i < lineWidth; ++i)
                setPixelAt(i, lineNum, line[i]);
        });
    }

    template<> template<>
    inline void Bitmap<ColorRGB>::draw<ColorRGB>(uint32_t const * otherBuffer, int otherWidth, int otherHeight, Point where, Rect fromRect) {
        // TODO determine the actual rectangle to draw        
        // TODO where possible do 32bit mem sets instead of per color via unrolling
        Color const * otherColor = reinterpret_cast<Color const *>(otherBuffer);
        for (int xx = 0, xe = fromRect.width(); xx != xe; ++xx)
            for (int yy = 0, ye = fromRect.height(); yy != ye; ++yy) 
                setPixelAt(where.x() + xx, where.y() + yy, otherColor[map(fromRect.left() + xx, fromRect.top() + yy, otherWidth, otherHeight)]);
    }

    template<>
    inline void Bitmap<Color256>::loadImage(PNG && png) {
        resize(png.width(), png.height());
        png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
            for (int i = 0; i < lineWidth; ++i) {
                ColorRGB c = line[i];
                setPixelAt(i, lineNum, Color::RGB(c.r(), c.g(), c.b()));
            }
        });
    }

    template<> template<>
    inline void Bitmap<Color256>::draw<Color256>(uint32_t const * otherBuffer, int otherWidth, int otherHeight, Point where, Rect fromRect) {
        // TODO determine the actual rectangle to draw        
        // TODO where possible do 32bit mem sets instead of per color via unrolling
        Color const * otherColor = reinterpret_cast<Color const *>(otherBuffer);
        for (int xx = 0, xe = fromRect.width(); xx != xe; ++xx)
            for (int yy = 0, ye = fromRect.height(); yy != ye; ++yy) 
                setPixelAt(where.x() + xx, where.y() + yy, otherColor[map(fromRect.left() + xx, fromRect.top() + yy, otherWidth, otherHeight)]);
    }

    template<> template<>
    inline void Bitmap<Color256>::draw<ColorRGB>(uint32_t const * otherBuffer, int otherWidth, int otherHeight, Point where, Rect fromRect) {
        // TODO determine the actual rectangle to draw        
        // TODO where possible do 32bit mem sets instead of per color via unrolling
        ColorRGB const * otherColor = reinterpret_cast<ColorRGB const *>(otherBuffer);
        for (int xx = 0, xe = fromRect.width(); xx != xe; ++xx)
            for (int yy = 0, ye = fromRect.height(); yy != ye; ++yy) 
                setPixelAt(where.x() + xx, where.y() + yy, Color::RGB(otherColor[map(fromRect.left() + xx, fromRect.top() + yy, otherWidth, otherHeight)]));
    }



} // namespace rckid