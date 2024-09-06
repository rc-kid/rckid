#pragma once

#include "bitmap.h"

namespace rckid {

    /** Sprite
     
        Sprite is pixel surface (width, height, buffer) coupled with position information (x, y). Palette is intended to be provided by the tile engine that the display belongs to. 
     */
    template<typename COLOR>
    class Sprite : public Surface<COLOR> {
    public:
        Sprite(Coord width, Coord height): Surface<COLOR>{width, height} {}

        Sprite(Sprite const &) = delete;

        Sprite(Sprite && from):
            Bitmap<COLOR>{std::move(from)}, 
            x_{from.x_},
            y_{from.y_},
            paletteOffset_{from.paletteOffset_} {
        }

        int x() const { return x_; }
        int y() const { return y_; }
        uint8_t paletteOffset() const { return paletteOffset_; }

        void setX(int x) { x_ = x; }
        void setY(int y) { y_ = y; }
        void setPos(int x, int y) { x_ = x; y_ = y; }
        void setPaletteOffset(uint8_t offset) { paletteOffset_ = offset; }

        /** */
        bool renderColumn(int x, ColorRGB * columnBuffer, int bufferHeight, ColorRGB const * palette) const {
/*            
            // determine the sprite column to render
            int rx = x - x_;
            // don't render at all if the sprite column is out of range
            if (rx < 0 || rx >= width_)
                return false;
            // see if we are rendeing the whole sprite
            if (y_ >= 0 && y_ <= bufferHeight - height_) {
                renderFullColumn(rx, columnBuffer + y_, palette);
            } else {
                // don't start 
                UNIMPLEMENTED;
            }
            return true;
        */
            return false;   
        }

    private:
 
        /*
         uint16_t * renderFullColumn(int x, ColorRGB * buffer, ColorRGB const * palette = nullptr) const {
            return convertToRGBtransparent<FMT>(
                getColumn<FMT>(pixelBuffer_, width_, height_, x),
                reinterpret_cast<uint16_t*>(buffer), 
                height_, 
                palette,
                paletteOffset_
            );
        }
        */

        Coord x_ = 0;
        Coord y_ = 0;
        uint8_t paletteOffset_ = 0;

    }; // rckid::Sprite

} // namespace rckid