#pragma once

#include <rckid/memory.h>
#include <rckid/buffer.h>
#include <rckid/graphics/geometry.h>
#include <rckid/graphics/color.h>
#include <rckid/graphics/blit.h>
#include <rckid/graphics/image_source.h>

namespace rckid {

    /** Rendering surface.
     
     */
    class Canvas {
    public:
        Canvas(Coord width, Coord height):
            w_{width},
            h_{height},
            pixels_{new Color::RGB565[width * height]} {
        }

        Coord width() const { return w_; }
        Coord height() const { return h_; }

        Color::RGB565 at(Coord x, Coord y) const { return pixels_.get()[mapIndexColumnFirst(x, y, w_, h_)]; }
        Color::RGB565 & at(Coord x, Coord y) { return pixels_.get()[mapIndexColumnFirst(x, y, w_, h_)]; }

        /** Fills rectangle with given color. 
         */
        void fill(Rect rect, Color::RGB565 color) {
            // default, very slow implementation
            for (int x = rect.left(), xe = rect.right(); x < xe; ++x)
                for (int y = rect.top(), ye = rect.bottom(); y < ye; ++y)
                    at(x, y) = color;
        }

        /** Fills the entire canvas with given color.
         */
        void fill(Color::RGB565 color) {
            memset16(reinterpret_cast<uint16_t *>(pixels_.get()), color, w_ * h_);
        }

        /** Writes text at given coordinates with specified font & color.
         */
        Writer text(Coord x, Coord y, Font font, Color color) {
            Color::RGB565 palette[4];
            font->createFontPalette(palette, color);
            return Writer{[this, x, y, font, color, palette, startx = x](char c) mutable {
                if (c != '\n') {
                    GlyphInfo const * gi = font->glyphInfoFor(c);
                    putChar(x, y, gi, font, palette);
                    x += gi->advanceX;
                } else {
                    y += font->size;
                    x = startx;

                }
            }};
        }

        Writer text(Coord x, Coord y, Font font, std::function<Color::RGB565(uint32_t)> color) {
            return Writer{[this, x, y, font, color, startx = x, charIndex = 0](char c) mutable {
                if (c != '\n') {
                    Color::RGB565 palette[4];
                    font->createFontPalette(palette, color(charIndex++));
                    GlyphInfo const * gi = font->glyphInfoFor(c);
                    putChar(x, y, gi, font, palette);
                    x += gi->advanceX;
                } else {
                    y += font->size;
                    x = startx;
                }
            }};
        }

        Writer textRainbow(Coord x, Coord y, Font font, uint16_t hueStart, int16_t hueInc) {
            auto getColor = [hueStart, hueInc](uint32_t) mutable {
                return Color::HSV(hueStart += hueInc, 255, 255);
            };
            return text(x, y, font, getColor);

        }

        /** Renders particular canvas column
         
            This is the main SDK rendering API making the canvas compatible with other elements, such as the UI widgets, and direct display rendering.
         */
        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) {
            ASSERT(column < width());
            ASSERT(startRow + numPixels <= height());
            Color::RGB565 const * start = pixels_.get() + mapIndexColumnFirst(column, 0, w_, h_) + startRow;
            blit_rgb565(reinterpret_cast<uint8_t const *>(start), buffer, numPixels);
        }

    private:

        void putChar(Coord x, Coord y, GlyphInfo const * gi, Font font, Color::RGB565 const * palette) {
            for (Coord xx = x + gi->advanceX; xx >= x; --x) {
                if (xx < 0)
                    break; // it won't be better
                if (xx >= w_)
                    continue;
                Color::RGB565 * buffer = pixels_.get() + mapIndexColumnFirst(xx, 0, w_, h_);
                if (y < 0)
                    font->renderColumn(xx - x, -y, w_ + y, gi, buffer, palette);
                else
                    font->renderColumn(xx - x, 0, w_ - y, gi, buffer + y, palette);
            }
        }

        Coord w_;
        Coord h_;
        unique_ptr<Color::RGB565> pixels_;
    }; // rckid::Canvas


    template<typename RESULT>
    class CanvasApp : public ModalApp<RESULT> {
    public:
        explicit CanvasApp(Rect rect):
            rect_{rect},
            canvas_{rect.width(), rect.height()},
            renderBuffer_{static_cast<uint32_t>(rect.height())}
        {

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