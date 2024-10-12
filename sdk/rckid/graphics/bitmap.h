#pragma once
#include <type_traits>

#include "../rckid.h"
#include "drawing.h"
#include "font.h"
#include "png.h"

namespace rckid {

    /** Pixel surface, templated by the underlying color type. 

        Surface constists of width and height information and a pixel buffer stored in the native orientation of the RCKid device, i.e. column first right to left, top to bottom. It provides access to the pixel data and drawing primitives via the drawing.h functions in two basic modes: slow per pixel manipulation and faster blitting of regions.
     */
    template<typename COLOR>
    class Surface {
    public:
        using Color = COLOR;
        static constexpr uint8_t BPP = Color::BPP;

        static_assert(BPP == 16 || BPP ==8 || BPP == 4);

        Surface() = default;
        Surface(Coord width, Coord height): w_{width}, h_{height}, buffer_{allocateBuffer(width, height) } {}

        Surface(Surface const &) = delete;
        Surface(Surface && from): 
            w_{from.w_}, 
            h_{from.h_}, 
            buffer_{from.buffer_} 
        {
            from.w_ = 0;
            from.h_ = 0;
            from.buffer_ = nullptr;
        }

        ~Surface() {
            delete [] buffer_;
        }

        Surface & operator = (Surface && from) {
            w_ = from.w_;
            h_ = from.h_;
            delete buffer_;
            buffer_ = from.buffer_;
            from.buffer_ = nullptr;
            return *this;
        }

        Coord width() const { return w_; }
        Coord height() const { return h_; }

        uint32_t numPixels() const { return w_ * h_; }

        /** Returns the color buffer. 
         
         */
        uint8_t const * buffer() const { return buffer_; }
        uint8_t * buffer() { return buffer_; }

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

        void blit(Point where, Surface const & src) { blit(where, src, Rect::WH(src.width(), src.height())); }

        void blit(Point where, Surface const & src, Rect srcRect) {
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
                    if (x < width())
                        x += putChar(Point{x, y}, font, c, colors);
                } else {
                    x = startX;
                    y += font.size;
                }
            }};
        }

        Writer text(int x, int y, Font const & font, Color color) {
            Color colors[] = { color, (color + 1), (color + 2) , (color + 3) };
            int startX = x;
            return Writer{[this, x, y, startX, font, colors](char c) mutable {
                if (c != '\n') {
                    if (x < width())
                        x += putChar(Point{x, y}, font, c, colors);
                } else {
                    x = startX;
                    y += font.size;
                }
            }};
        }

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

        static Surface fromImage(PNG && png) {
            Surface result{png.width(), png.height()};
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

    }; // rckid::Surface

    template<>
    inline Writer Surface<ColorRGB>::text(int x, int y, Font const & font, ColorRGB color) {
        Color colors[] = {
            color.withAlpha(0), 
            color.withAlpha(85), 
            color.withAlpha(170), 
            color.withAlpha(255), 
        };
        int startX = x;
        return Writer{[this, x, startX, y, font, colors](char c) mutable {
            if (c != '\n') {
                if (x < width())
                    x += putChar(Point{x, y}, font, c, colors);
            } else {
                x = startX;
                y += font.size;
            }
        }};
    }

    /** Bitmap
     
        Bitmap is pixel surface with its drawing primitives and pixel buffer management paired with a palette holder for the given bitmap (if palettes are used). As such, Bitmap contains all the information needed to be rendered and provides a renderer specialization (below). 
     */
    template<typename COLOR>
    class Bitmap : public Surface<COLOR>, public PaletteHolder<COLOR> {
    public:

        Bitmap() = default;
        Bitmap(Coord width, Coord height): Surface<COLOR>{width, height} {}

        Bitmap(Bitmap const &) = delete;
        Bitmap(Bitmap && from) = default;

    }; // rckid::Bitmap


    /** Bitmap renderer
      
        Renders the bitmap in a column-wise manner starting from right to left. Uses double buffering so that while one column is being sent to the display, another column is being prepared, which means converting the bitmap colors to ColorRGB using the palette (or just memcopy if using ColorRGB). 
     */
    template<typename T>
    class Renderer<Bitmap<T>> {
    public:
        void initialize(Bitmap<T> const & bitmap) {
            displaySetMode(DisplayMode::Native);
            displaySetUpdateRegion(bitmap.width(), bitmap.height());
            // allocate enough room for 2 columns
            renderBuffer_ = new ColorRGB[bitmap.height() * 2];
        }

        void finalize() {
            // delete the buffer
            delete renderBuffer_;
            renderBuffer_ = nullptr;
        }

        void render(Bitmap<T> const & bitmap) {
            column_ = bitmap.width() - 1;
            renderColumn(column_, bitmap);
            renderColumn(column_ - 1, bitmap);
            displayWaitVSync();
            displayUpdate(getRenderBufferChunk(column_, bitmap), bitmap.height(), [&]() {
                // move to previous (right to left column), if there is none, we are done rendering
                if (--column_ < 0)
                    return; 
                // render the column we currently point to (already in the renderBuffer)
                displayUpdate(getRenderBufferChunk(column_, bitmap), bitmap.height());
                // render the column ahead, if any
                if (column_ > 0)
                    renderColumn(column_ - 1, bitmap);
            });
        }

    private:

        ColorRGB * getRenderBufferChunk(Coord column, Bitmap<T> const & bitmap) { 
            return renderBuffer_ + (column % 1) * bitmap.height(); 
        }

        void renderColumn(Coord column, Bitmap<T> const & bitmap) {
            ColorRGB * rb = getRenderBufferChunk(column, bitmap);
            uint8_t const * sb = bitmap.buffer() + pixelBufferColumnOffset<T>(bitmap.width(), bitmap.height(), column);
            pixelBufferToRGB<T>(sb, rb, bitmap.height(), bitmap.palette(), 0);
        }

        int column_; 
        ColorRGB * renderBuffer_ = nullptr;

    }; // rckid::Renderer<Bitmap<T>>

    /** The simplest display renderer from a RGB color bitmap. 
     
        This specialization renders the entire buffer in one display update as the buffer contains directly the data to be sent over the display data lanes. 
     */
    template<>
    class Renderer<Bitmap<ColorRGB>> {
    public:
        void initialize(Bitmap<ColorRGB> const & bitmap) {
            displaySetMode(DisplayMode::Native);
            displaySetUpdateRegion(bitmap.width(), bitmap.height());
        }

        void finalize() {
            // nothing to finalize
        }

        void render(Bitmap<ColorRGB> const & bitmap) {
            displayWaitVSync();
            displayUpdate(reinterpret_cast<ColorRGB const*>(bitmap.buffer()), bitmap.numPixels());
        }

    }; // rckid::Renderer<Bitmap<ColorRGB>>

} // namespace rckid