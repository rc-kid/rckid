#pragma once

#include "../rckid.h"
#include "../app.h"
#include "surface.h"
#include "font.h"
#include "../assets/fonts/Iosevka16.h"

namespace rckid {

    template<typename PIXEL>
    class Canvas : protected Surface<PIXEL::BPP> {
    public:
        using Pixel = PIXEL;
        static constexpr uint32_t BPP = PIXEL::BPP;

        /** Default constructor that creates an empty bitmap with no pixel buffer
         */
        Canvas():
            pixels_{nullptr}, w_{0}, h_{0} {
        }

        /** Creates the bitmap.
         */
        Canvas(Coord w, Coord h): pixels_{new uint16_t[numHalfWords(w, h)]}, w_{w}, h_{h} {
        }

        Canvas(Canvas const &) = delete;

        Canvas(Canvas && other) noexcept: pixels_{other.pixels_}, w_{other.w_}, h_{other.h_} {
            other.pixels_ = nullptr;
            other.w_ = 0;
            other.h_ = 0;
        }

        Canvas & operator = (Canvas const &) = delete;
        Canvas & operator = (Canvas && other) noexcept {
            if (this != &other) {
                delete [] pixels_;
                pixels_ = other.pixels_;
                w_ = other.w_;
                h_ = other.h_;
                other.pixels_ = nullptr;
                other.w_ = 0;
                other.h_ = 0;
            }
            return *this;
        }

        /** Frees the bitmap.
         */
        ~Canvas() { 
            delete [] pixels_; 
        };

        bool empty() const { return pixels_ == nullptr; }

        Coord width() const { return w_; }
        Coord height() const { return h_; }
        uint32_t numPixels() const { return w_ * h_; }
        uint32_t numBytes() const { return numBytes(w_, h_); }
        uint32_t numHalfWords() const { return numHalfWords(w_, h_); }

        // image loading
        void loadImage(ImageDecoder && decoder, Point at = {0, 0});

        // single pixel access

        Pixel at(Coord x, Coord y) const { return Pixel::fromRaw(pixelAt(x, y, w_, h_, pixels_)); }
        void setAt(Coord x, Coord y, Pixel c) { setPixelAt(x, y, w_, h_, pixels_, c.toRaw()); }

        /** Access to pixels & column pixels for direct rendering. 
         
            The column pixels are useful for column-base rendering into bitmaps (such as in the case of UI framebuffer rendering). The pixels() method returns the entire pixel array, while the columnPixels() method returns a pointer to the start of the column in the pixel array. The column is in native display orientation, i.e. right-top corner is index 0, column-first format.

            Note that the non-const versions are potentially dangerous if ROM backed bitmaps are used. 
         */
        uint16_t const * pixels() const { return pixels_; }
        uint16_t const * columnPixels(Coord column) const { return pixels_ + columnOffset(column, w_, h_); }
        uint16_t * pixels() { return pixels_; }
        uint16_t * columnPixels(Coord column) { return pixels_ + columnOffset(column, w_, h_); }

        /** Renders given bitmap column. 
         */
        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, uint16_t * buffer, uint16_t const * palette = nullptr) const {
            return renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, palette);
        }

        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, uint16_t * buffer, uint32_t transparent, uint16_t const * palette = nullptr) const {
            return renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, transparent, palette);
        }


        Pixel fg() const { return fg_; }

        void setFg(Pixel color) {
            fg_ = color;
            fontColors_ = Font::colorToArray(fg_);
        }




        Font const & font() const { return font_; }

        void setFont(Font const & font) {
            font_ = font;
        }   

        Writer text(Coord x, Coord y, Font const & font, Pixel color) {
            setFg(color);
            font_ = font;
            textStartX_ = x;
            textX_ = x;
            textY_ = y;
            return Writer{[](char c, void * arg) {
                Canvas * self = reinterpret_cast<Canvas*>(arg);
                if (c != '\n') {
                    if (self->textX_ < self->w_) {
                        self->textX_ += self->putChar(self->textX_, self->textY_, self->w_, self->h_, self->font_, c, self->fontColors_.data(), self->pixels_);
                    }
                } else {
                    self->textX_ = self->textStartX_;
                    self->textY_ += self->font_.size;
                }
            }, this};
        }        

        void fill(Pixel color) { fill(color, Rect::WH(w_, h_)); }

        void fill(Pixel color, Rect rect) {            
            // default, very slow implementation
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setAt(x, y, color);
        }

        void clear() {
            delete [] pixels_;
            pixels_ = nullptr;
            w_ = 0;
            h_ = 0;
        }

        void resize(Coord w, Coord h) {
            clear();
            pixels_ = new uint16_t[numHalfWords(w, h)];
            w_ = w;
            h_ = h;
        }

    private:

        using Surface<BPP>::pixelAt;
        using Surface<BPP>::setPixelAt;
        using Surface<BPP>::columnOffset;
        using Surface<BPP>::numHalfWords;
        using Surface<BPP>::renderColumn;
        using Surface<BPP>::putChar;

        uint16_t * pixels_;
        Coord w_;
        Coord h_;


        Pixel fg_;
        Font font_ = Font::fromROM<assets::Iosevka16>();

        std::array<uint16_t, 4> fontColors_;
        // capture for current text out coordinates Writers allow only a single void * argument which is this
        Coord textStartX_ = 0;
        Coord textX_ = 0;
        Coord textY_ = 0;
        
    }; // Canvas

    /** Canvas that can render itself. 
     
        Defined as template and specialized based on used color. See the specializations below for more information.  
     */
    template<typename PIXEL>
    class RenderableCanvas : public Canvas<PIXEL> {

    }; 

    /** Renderable canvas specialization for Full 16bit RGB colors. 
     
        This is rather simple as the entire pixel array of the canvas can be sent directly to the displayUpdate() method. Initialization simply sets the display to native mode and sets update region to a centered rectangle of the canvas' size. There is no need to finalize anything. 
      */
    template<>
    class RenderableCanvas<ColorRGB> : public Canvas<ColorRGB> {
    public:
        using Canvas<ColorRGB>::width;
        using Canvas<ColorRGB>::height;
        using Canvas<ColorRGB>::pixels;
        using Canvas<ColorRGB>::numPixels;
        using Canvas<ColorRGB>::Canvas;

        RenderableCanvas():
            Canvas{RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT} {
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

    template<typename PIXEL, typename T = void>
    using CanvasApp = RenderableApp<RenderableCanvas<PIXEL>, T>;


} // namespace rckid