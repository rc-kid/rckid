#pragma once

#include <type_traits>

#include "bitmap.h"

namespace rckid {

    /** Sprite
     
        Sprite is a bitmap (width, height, buffer) coupled with position information (x, y). Palette is intended to be provided by the tile engine that the display belongs to. 
     */
    template<typename COLOR>
    class Sprite : public Bitmap<COLOR> {
    public:
        Sprite(Coord width, Coord height): Bitmap<COLOR>{width, height} {}

        Sprite(Sprite const &) = delete;

        Sprite(Sprite && from):
            Bitmap<COLOR>{std::move(from)}, 
            x_{from.x_},
            y_{from.y_},
            paletteOffset_{from.paletteOffset_} {
        }

        int x() const { return x_; }
        int y() const { return y_; }

        void setX(int x) { x_ = x; }
        void setY(int y) { y_ = y; }
        void setPos(int x, int y) { x_ = x; y_ = y; }

        uint8_t paletteOffset() const { return paletteOffset_; }

        void setPaletteOffset(uint8_t offset) { paletteOffset_ = offset; }


        /** */
        bool renderColumn(int x, ColorRGB * columnBuffer, int bufferHeight, ColorRGB const * palette) const {
            // determine the sprite column to render
            int rx = x - x_;
            // don't render at all if the sprite column is out of range
            if (rx < 0 || rx >= width())
                return false;
            // see if we are rendeing the whole sprite
            if (y_ >= 0 && y_ <= bufferHeight - height()) {
                renderFullColumn(rx, columnBuffer + y_, palette);
            } else {
                // don't start 
                UNIMPLEMENTED;
            }
            return true;
        }

        // necessary to bring template parent methods to scope
        using Bitmap<COLOR>::width;
        using Bitmap<COLOR>::height;

    private:
 
        ColorRGB * renderFullColumn(Coord x, ColorRGB * buffer, ColorRGB const * palette = nullptr) const {
            return pixelBufferToRGBTransparent<COLOR>(
                this->buffer() + pixelBufferColumnOffset<COLOR>(width(), height(), x),
                buffer, 
                height(), 
                palette,
                paletteOffset_
            );
        }

        Coord x_ = 0;
        Coord y_ = 0;
        uint8_t paletteOffset_ = 0;

    }; // rckid::Sprite


    template<>
    class Sprite<ColorRGB> : public Bitmap<ColorRGB> {
        Sprite(Coord width, Coord height): Bitmap<ColorRGB>{width, height} {}

        Sprite(Sprite const &) = delete;

        Sprite(Sprite && from):
            Bitmap<ColorRGB>{std::move(from)}, 
            x_{from.x_},
            y_{from.y_},
            transparentColor_{from.transparentColor_} {
        }

        int x() const { return x_; }
        int y() const { return y_; }

        void setX(int x) { x_ = x; }
        void setY(int y) { y_ = y; }
        void setPos(int x, int y) { x_ = x; y_ = y; }

        ColorRGB transparentColor() const { return transparentColor_; }

        void transparentColor(ColorRGB color) { transparentColor_ = color; }

        /** */
        bool renderColumn(int x, ColorRGB * columnBuffer, int bufferHeight, ColorRGB const * palette) const {
            ASSERT(palette == nullptr);
            // determine the sprite column to render
            int rx = x - x_;
            // don't render at all if the sprite column is out of range
            if (rx < 0 || rx >= width())
                return false;
            // see if we are rendeing the whole sprite
            if (y_ >= 0 && y_ <= bufferHeight - height()) {
                renderFullColumn(rx, columnBuffer + y_);
            } else {
                // don't start 
                UNIMPLEMENTED;
            }
            return true;
        }

    private:
 
        ColorRGB * renderFullColumn(int x, ColorRGB * buffer) const {
            return pixelBufferToRGBTransparent(
                this->buffer() + pixelBufferColumnOffset<ColorRGB>(width(), height(), x),
                buffer, 
                height(), 
                transparentColor_
            );
        }

        Coord x_ = 0;
        Coord y_ = 0;
        ColorRGB transparentColor_;

    }; // rckid::Sprite<ColorRGB>


} // namespace rckid