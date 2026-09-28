#pragma once

#include <rckid/memory.h>
#include <rckid/buffer.h>
#include <rckid/graphics/geometry.h>
#include <rckid/graphics/color.h>
#include <rckid/graphics/blit.h>
#include <rckid/graphics/bitmap.h>
#include <rckid/graphics/image_source.h>

namespace rckid {

    /** Canvas is editable bitmap. 
     */
    class Canvas {
    public:

        Canvas(Coord width, Coord height, Color::Representation colorRepresentation = Color::Representation::RGB565):
            w_{width},
            h_{height},
            colorRepresentation_{colorRepresentation},
            pixels_{new uint8_t[Color::getPixelArraySize(colorRepresentation, width, height)]} {
        }

        Canvas(Bitmap && bmp):
            w_{bmp.width()},
            h_{bmp.height()},
            colorRepresentation_{bmp.colorRepresentation()},
            pixels_{std::move(bmp).detachPixelArray().releaseOrCopy()}
        {
            ASSERT(Heap::contains(pixels_.get()));
        }

        Coord width() const { return w_; }
        Coord height() const { return h_; }

        Color::Representation colorRepresentation() const { return colorRepresentation_; }

        uint32_t bpp() const { return colorRepresentationBpp(colorRepresentation_); }

        Color::RGB565 const * palette() const { return palette_.get(); }

        void setPalette(unique_ptr<Color::RGB565> palette) {
            palette_ = std::move(palette);
        }

        uint16_t getPixel(Coord x, Coord y) const {
            return Color::getPixel(colorRepresentation_, pixels_.get(), w_, h_, x, y);
        }

        uint16_t getPixel(Point pos) const { return getPixel(pos.x, pos.y); }

        void setPixel(Coord x, Coord y, uint16_t rawValue) {
            if (x < 0 || x >= w_)
                return;
            if (y < 0 || y >= h_)
                return;
            Color::setPixel(colorRepresentation_, pixels_.get(), w_, h_, x, y, rawValue);
        } 

        void setPixel(Point pos, uint16_t rawValue) { setPixel(pos.x, pos.y, rawValue); }

        /** Fills rectangle with given color. 
         */
        void fill(Rect rect, uint16_t color) {
            // default, very slow implementation
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    setPixel(x, y, color);
        }

        void fill(uint16_t color) {
            switch (colorRepresentation_) {
                case Color::Representation::RGB565:
                    memset16(reinterpret_cast<uint16_t *>(pixels_.get()), color, w_ * h_);
                    break;
                default:
                    UNIMPLEMENTED;
            }
        }

        /** \name Blitting

            Copies pixels of the given bitmap, or a part of another canvas to the canvas at given position. The source is clipped to its own bounds and then to the bounds of the canvas so that any position (including negative and out of bounds coordinates) is safe to use.

            As blitting reuses the renderColumn API of the source, all color representations, palettes and transparent colors supported by the SDK work out of the box. The destination canvas must be RGB565 (which is the canvas default).
         */
        //@{
        void blit(Point where, Bitmap & src) {
            blit(where, src, Rect::WH(src.width(), src.height()));
        }

        void blit(Point where, Bitmap & src, Rect srcRect) {
            ASSERT(colorRepresentation_ == Color::Representation::RGB565);
            if (! clipBlit(where, srcRect, src.width(), src.height()))
                return;
            for (Coord i = 0; i < srcRect.width(); ++i)
                src.renderColumn(srcRect.x + i, srcRect.y, columnBuffer(where.x + i) + where.y, srcRect.height());
        }

        /** Copies a rectangle of another canvas (which can be the canvas itself) to given position.

            When blitting a canvas onto itself the overlapping regions are handled correctly (the semantics are that of a memmove, not memcpy).
         */
        void blit(Point where, Canvas const & src, Rect srcRect) {
            ASSERT(colorRepresentation_ == Color::Representation::RGB565);
            ASSERT(src.colorRepresentation_ == Color::Representation::RGB565);
            if (! clipBlit(where, srcRect, src.w_, src.h_))
                return;
            // if we are blitting on top of ourselves and the destination is to the right of the source, the columns must be copied back to front so that we never overwrite a column we have not read yet
            bool backwards = (& src == this) && (where.x > srcRect.x);
            for (Coord n = srcRect.width(), j = 0; j < n; ++j) {
                Coord i = backwards ? (n - j - 1) : j;
                memmove(
                    columnBuffer(where.x + i) + where.y,
                    src.columnBuffer(srcRect.x + i) + srcRect.y,
                    static_cast<size_t>(srcRect.height()) * sizeof(Color::RGB565)
                );
            }
        }
        //@}

        /** Writes text at given coordinates with specified font & color.
         */
        Writer text(Coord x, Coord y, Font font, uint16_t color) {
            switch (colorRepresentation_) {
                case Color::Representation::RGB565: {
                    Color::RGB565 palette[4];
                    font->createFontPalette(palette, Color::RGB565{color});
                    return Writer{[this, x, y, font, color, palette, startx = x](char c) mutable {
                        if (c != '\n') {
                            GlyphInfo const * gi = font->glyphInfoFor(c);
                            putChar16(x, y, gi, font, palette);
                            x += gi->advanceX;
                        } else {
                            y += font->size;
                            x = startx;
                        }
                    }};
                    break;
                }
                default:
                    UNIMPLEMENTED;
            }
        }

        Writer text(Coord x, Coord y, Font font, Color color) {
            switch (colorRepresentation_) {
                case Color::Representation::RGB565:
                    return text(x, y, font, static_cast<uint16_t>(Color::RGB565{color}));
                default:
                    UNIMPLEMENTED;
            }
        }

        Writer text(Coord x, Coord y, Font font, std::function<uint16_t(uint32_t)> color) {
            switch (colorRepresentation_) {
                case Color::Representation::RGB565: {
                    return Writer{[this, x, y, font, color, startx = x, charIndex = 0](char c) mutable {
                        if (c != '\n') {
                            Color::RGB565 palette[4];
                            font->createFontPalette(palette, Color::RGB565{color(charIndex++)});
                            GlyphInfo const * gi = font->glyphInfoFor(c);
                            putChar16(x, y, gi, font, palette);
                            x += gi->advanceX;
                        } else {
                            y += font->size;
                            x = startx;
                        }
                    }};
                }
                default:
                    UNIMPLEMENTED;
            }
        }


        Writer textRainbow(Coord x, Coord y, Font font, uint16_t hueStart, int16_t hueInc) {
            auto getColor = [hueStart, hueInc](uint32_t) mutable {
                return Color::RGB565{Color::HSV(hueStart += hueInc, 255, 255)};
            };
            return text(x, y, font, getColor);
        }

        /** Renders particular canvas column
         
            This is the main SDK rendering API making the canvas compatible with other elements, such as the UI widgets, and direct display rendering.
         */
        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) {
            ASSERT(column < width());
            ASSERT(startRow + numPixels <= height());
            // get source start pointer
            uint8_t const * start = rawPixelArray(column) + startRow * bpp() / 8;
            switch (colorRepresentation_) {
                case Color::Representation::RGB565:
                    return blit_rgb565(start, buffer, numPixels);
                case Color::Representation::RGB332:
                    return blit_rgb332(start, buffer, numPixels);
                case Color::Representation::Index256:
                    return blit_index256(start, buffer, numPixels, palette_.get());
                case Color::Representation::Index16:
                    return blit_index16(start, buffer, numPixels, palette_.get(), startRow % 2);
            }
            UNREACHABLE;
        }

        /** Saves the canvas as Raw format stream
         
            TODO hacky for the game engine demo
         */
        void saveAsRaw(WriteStream & s) {
            ASSERT(colorRepresentation_ == Color::Representation::RGB565);
            s.write(reinterpret_cast<uint8_t *>(pixels_.get()), w_ * h_ * 2);
            s.binaryWriter()
                << static_cast<uint16_t>(w_)
                << static_cast<uint16_t>(h_);
        }

        /** Returns pointer to the first row of the given column of an RGB565 canvas. 
         
            Rows within a column are contiguous, so this is the buffer the SDK renderColumn() API writes into, which is what makes the blitting routines above simple and what allows arbitrary renderable objects (bitmaps, tile grids, UI widgets) to be drawn into the canvas.
         */
        Color::RGB565 * columnBuffer(Coord column) const {
            ASSERT(colorRepresentation_ == Color::Representation::RGB565);
            ASSERT(column >= 0 && column < w_);
            return reinterpret_cast<Color::RGB565 *>(pixels_.get()) + mapIndexColumnFirst(column, 0, w_, h_);
        }

    private:

        uint8_t const * rawPixelArray(Coord column = 0) const {
            return pixels_.get() + mapIndexColumnFirst(column, 0, w_, h_) * bpp() / 8;
        }

        /** Clips a blit operation against the source and the destination (this canvas).

            Updates both the destination position and the source rectangle in place and returns false if there is nothing left to draw.
         */
        bool clipBlit(Point & where, Rect & srcRect, Coord srcWidth, Coord srcHeight) const {
            srcRect = srcRect.intersectWith(Rect::WH(srcWidth, srcHeight));
            if (srcRect.empty())
                return false;
            // clip against the left & top edge of the canvas, moving the source rectangle accordingly
            if (where.x < 0) {
                srcRect.x -= where.x;
                srcRect.w += where.x;
                where.x = 0;
            }
            if (where.y < 0) {
                srcRect.y -= where.y;
                srcRect.h += where.y;
                where.y = 0;
            }
            // and against the right & bottom edge, where only the size changes
            if (where.x + srcRect.w > w_)
                srcRect.w = w_ - where.x;
            if (where.y + srcRect.h > h_)
                srcRect.h = h_ - where.y;
            return ! srcRect.empty();
        }

        void putChar16(Coord x, Coord y, GlyphInfo const * gi, Font font, Color::RGB565 const * palette) {
            for (Coord xx = x + gi->advanceX; xx >= x; --xx) {
                if (xx < 0)
                    break; // it won't be better
                if (xx >= w_)
                    continue;
                Color::RGB565 * buffer = reinterpret_cast<Color::RGB565* >(pixels_.get()) + mapIndexColumnFirst(xx, 0, w_, h_);
                if (y < 0)
                    font->renderColumn(xx - x, -y, w_ + y, gi, buffer, palette);
                else
                    font->renderColumn(xx - x, 0, w_ - y, gi, buffer + y, palette);
            }
        }

        Coord w_ = 0;
        Coord h_ = 0;
        Color::Representation colorRepresentation_ = Color::Representation::RGB565;
        unique_ptr<uint8_t> pixels_;
        unique_ptr<Color::RGB565> palette_;

    }; // rckid::Canvas

    /** Application that renders its contents into a canvas. 
     
        One of the simplest application types, the canvas app is backed by a framebuffer canvas that can be updated at each loop iteration and is then sent to the display. This is super simple, retains state between frames, which can lead to faster draw times with only changes being updated at every frame. 
     */
    template<typename RESULT>
    class CanvasApp : public ModalApp<RESULT> {
    public:
        explicit CanvasApp(Rect rect):
            rect_{rect},
            canvas_{rect.width(), rect.height()},
            renderBuffer_{static_cast<uint32_t>(rect.height())} {
        }

        CanvasApp(): CanvasApp{Rect::WH(display::WIDTH, display::HEIGHT)} {}

        Rect rect() const { return rect_; }

        Coord width() const { return rect_.width(); }
        Coord height() const { return rect_.height(); }
    
    protected:

        Canvas & canvas() { return canvas_; }
        
        void onFocus() override {
            ModalApp<RESULT>::onFocus();
            display::enable(rect_, hal::display::RefreshDirection::ColumnFirst);
            // deal with header
        }

        void render() override {
            renderCol_ = width() - 1;
            hal::display::update([this](Color::RGB565 * & buffer, uint32_t & bufferSize) {
                if (renderCol_ < 0) {
                    buffer = nullptr;
                    return;
                } else if (buffer == nullptr) {
                    buffer = renderBuffer_.front().data();
                    bufferSize = height();
                    ASSERT(bufferSize <= renderBuffer_.size());
                    renderBuffer_.swap();
                }
                canvas_.renderColumn(renderCol_--, 0, buffer, height());
            });
        }

    private:
        Rect rect_;
        Canvas canvas_;

        DoubleBuffer<Color::RGB565> renderBuffer_;
        Coord renderCol_;
    
    }; // rckid::CanvasApp

} // namespace rckid