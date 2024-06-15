#pragma once

#include "tile.h"

namespace rckid {

    /** Sprite 
     */
    template<PixelFormat FMT>
    class Sprite {
    public:
        Sprite(int width, int height):
            width_{width}, 
            height_{height},
            pixelBuffer_{ new uint32_t[pixelBufferLength<FMT>(width, height)]} {
        }

        Sprite(Sprite const &) = delete;

        Sprite(Sprite && from):
            x_{from.x_},
            y_{from.y_},
            width_{from.width_},
            height_{from.height_},
            paletteOffset_{from.paletteOffset_},
            pixelBuffer_{from.pixelBuffer_} {
            from.width_ = 0;
            from.height_ = 0;
            from.pixelBuffer_ = nullptr;
        }

        ~Sprite() {
            delete [] pixelBuffer_;
        }

        int width() const { return width_; }
        int height() const { return height_; }
        int x() const { return x_; }
        int y() const { return y_; }
        uint8_t paletteOffset() const { return paletteOffset_; }
        void setX(int x) { x_ = x; }
        void setY(int y) { y_ = y; }
        void setPos(int x, int y) { x_ = x; y_ = y; }
        void setPaletteOffset(uint8_t offset) { paletteOffset_ = offset; }


        unsigned pixelAt(int x, int y) const { return rckid::pixelAt<FMT>(pixelBuffer_, width_, height_, x, y); }
        void setPixelAt(int x, int y, unsigned c) { rckid::setPixelAt<FMT>(pixelBuffer_, width_, height_, x, y, c); }

        void fill(uint16_t color) {
            rckid::fillBuffer<FMT>(pixelBuffer_, pixelBufferLength<FMT>(width_, height_), color);
        }

        /** */
        bool renderColumn(int x, uint16_t * columnBuffer, int bufferHeight, ColorRGB const * palette) const {
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
        }


    private:
 
         uint16_t * renderFullColumn(int x, uint16_t * buffer, ColorRGB const * palette = nullptr) const {
            return convertToRGBtransparent<FMT>(
                getColumn<FMT>(pixelBuffer_, width_, height_, x),
                buffer, 
                height_, 
                palette,
                paletteOffset_
            );
        }
 
        int x_ = 0;
        int y_ = 0; 
        int width_;
        int height_; 
        uint8_t paletteOffset_ = 0;
        uint32_t * pixelBuffer_;
    }; 

    // TODO do we need sprite with static size? 

} // namespace rckid