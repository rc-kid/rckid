#pragma once

#include "../rckid.h"
#include "primitives.h"

namespace rckid {

    template<typename COLOR>
    class Bitmap {
    public:

        using Color = COLOR;

        static Bitmap onHeap(int width, int height) { 
            return Bitmap{width, height, new uint32_t[width * height * Color::BPP / 32]}; 
        }

        static Bitmap inVRAM(int width, int height) { 
            return Bitmap{width, height, static_cast<uint32_t*>(allocateVRAM(width * height * Color::BPP / 8))};
        }

        static Bitmap fromPNGonHeap(PNG && png) {
            int w = png.width();
            int h = png.height();
            Bitmap result = Bitmap::onHeap(w, h);
            result.loadPNG(std::move(png), Point{0,0});
            return result;
        }

        static Bitmap fromPNGinVRAM(PNG && png) {
            int w = png.width();
            int h = png.height();
            Bitmap result = Bitmap::inVRAM(w, h);
            result.loadPNG(std::move(png), Point{0,0});
            return result;
        }

        Bitmap() = default;
        Bitmap(int w, int h):w_{w}, h_{h} {}

        /** A very dangerous bitmap constructor with explicitly supplied memory buffer. The onHeap and inVRAM static methods should almost always be used instead, or the non-allocating constructors should be followed with explicit resize call or other means. 
        */
        Bitmap(int w, int h, uint32_t * buffer): w_{w}, h_{h}, buffer_{buffer} {
            ASSERT(w * Color::BPP % 32 == 0);
            ASSERT(h * Color::BPP % 32 == 0);
        }

        Bitmap(Bitmap && other):
            w_{other.w_},
            h_{other.h_} {
            buffer_ = other.buffer_;
            other.buffer_ = nullptr;
        }

        ~Bitmap() {
            if (! isVRAMPtr(buffer_))
                delete [] buffer_;
        }

        Bitmap & operator = (Bitmap && other) {
            if (! isVRAMPtr(buffer_))
                delete [] buffer_;
            w_ = other.w_;
            h_ = other.h_;
            buffer_ = other.buffer_;
            other.buffer_ = nullptr;
            return *this;
        }

        bool isValid() const { return buffer_ != nullptr; }

        int width() const { return w_; }
        int height() const { return h_; }

        size_t numPixels() const { return w_ * h_; }

        Color pixelAt(int x, int y) const {
            return reinterpret_cast<Color const *>(buffer_)[map(x, y)];
        }

        void setPixelAt(int x, int y, Color c) {
            if (x > 0 && x < width() && y > 0 && y < height())
                reinterpret_cast<Color*>(buffer_)[map(x, y)] = c;
        }

        /** \name Drawing operations

            Methods for drawing bitmaps. 
         */
        //@{
        template<typename SOURCE_COLOR>
        void draw(Bitmap<SOURCE_COLOR> const & from, int x, int y) {
            draw<SOURCE_COLOR>(from.buffer_, from.w_, from.h_, Point{x, y}, Rect::WH(from.w_, from.h_));
        }

        template<typename SOURCE_COLOR>
        void draw(Bitmap<SOURCE_COLOR> const & from, Point where) {
            draw<SOURCE_COLOR>(from.buffer_, from.w_, from.h_, where, Rect::WH(from.w_, from.h_));
        }

        template<typename SOURCE_COLOR>
        void draw(Bitmap<SOURCE_COLOR> const & from, int x, int y, Rect fromRect) {
            draw<SOURCE_COLOR>(from.buffer_, from.w_, from.h_, Point{x, y}, fromRect);
        }   

        template<typename SOURCE_COLOR>
        void draw(Bitmap<SOURCE_COLOR> const & from, Point where, Rect fromRect) {
            draw<SOURCE_COLOR>(from.buffer_, from.w_, from.h_, where, fromRect);
        }
        //@}

        /** \name Fill 
         */
        //@{

        void fill(Rect rect, Color color) {
            // TODO this is very slow
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setPixelAt(x, y, color);
        }

        //@}


        /** Loads PNG image into the canvas from given top left coordinates. 
         */
        void loadImage(PNG && png, Point from = Point{0,0});

        // TODO make this protected - we don't really want resizing to be public
        void resize(int w, int h, uint32_t * buffer) {
            ASSERT(w * Color::BPP / 32 == 0);
            ASSERT(h * Color::BPP / 32 == 0);
            if (isVRAMPtr(buffer_))
                delete [] buffer_;
            w_ = w;
            h_ = h;
            buffer_ = buffer;
        }

        Color * rawBuffer() { return reinterpret_cast<Color*>(buffer_); }

    protected:

        static __force_inline size_t map(int x, int y, int w, int h) { return (w - x - 1) * h + y; }
        constexpr __force_inline size_t map(int x, int y) const { return map(x, y, w_, h_); }

        void setBuffer(Bitmap && other) {
            if (!isVRAMPtr(buffer_))
                delete [] buffer_;
            w_ = other.w_;
            h_ = other.h_;
            buffer_ = other.buffer_;
            other.buffer_ = nullptr;
        }

        /** Basic bitmap drawing and color conversion function. Takes the source bitmap, which may, or may not be the same color, and draws it on own canvas starting from _where_. The portion of source bitmap to be drawn is specified in the _fromRect_. This is the general, unoptimized implementation for all cases, but more performant specializations are provided for special cases below. 
         */
        template<typename SOURCE_COLOR>
        void draw(uint32_t const * sourceBuffer, int sourceWidth, int sourceHeight, Point where, Rect fromRect) {
            // TODO 
        }

        int w_ = 0;
        int h_ = 0;
        uint32_t * buffer_ = nullptr;
    }; // rckid::Bitmap


    template<>
    inline void Bitmap<ColorRGB>::loadImage(PNG && png, Point where) {
        png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
            for (int i = 0; i < lineWidth; ++i)
                setPixelAt(i + where.x(), lineNum + where.y(), line[i]);
        });
    }

    template<>
    inline void Bitmap<Color256>::loadImage(PNG && png, Point where) {
        png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
            for (int i = 0; i < lineWidth; ++i) {
                ColorRGB c = line[i];
                setPixelAt(i + where.x(), lineNum + where.y(), Color::RGB(c.r(), c.g(), c.b()));
            }
        });
    }


#ifdef FOO

    template<typename COLOR>
    class Bitmap {
    public:
        using Color = COLOR;

        Bitmap(int w, int h) {
            allocateBuffer(w, h);
        }

        Bitmap(int w, int h, uint32_t * buffer):
            w_{w},
            h_{h},
            buffer_{buffer} {
            ASSERT(w % 4 == 0);
            ASSERT(h % 4 == 0);
        }

        Bitmap(Bitmap const &) = delete;

        Bitmap(Bitmap && other):
            w_{other.w_}, 
            h_{other.h_},
            buffer_{other.buffer_} {
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

        ~Bitmap() { 
            if (!isVRAMPtr(buffer_))
                delete [] buffer_; 
        }

        int width() const { return w_; }
        int height() const { return h_; }

        void resize(int width, int height) {
            if (width != w_ || height != h_) {
                delete [] buffer_;
                allocateBuffer(width, height);
            }
        }

        void resize(int width, int height, uint32_t * buffer) {
            if (! isVRAMPtr(buffer_))
                delete buffer_;
            buffer_ = buffer;
            w_ = width;
            h_ = height;
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

        void fill(Rect rect, Color color) {
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setPixelAt(x, y, color);
        }

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

        int w_;
        int h_;
        uint32_t * buffer_;
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


#endif

} // namespace rckid