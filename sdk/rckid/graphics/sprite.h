#pragma once

#include "bitmap.h"

namespace rckid {

    template<typename COLOR>
    class Sprite : public Bitmap<COLOR> {
    public:
        Sprite(Coord width, Coord height): Bitmap(width, height) {}

        Sprite(Sprite const &) = delete;

        Sprite(Sprite && from):
            Bitmap(std::move(from)), 
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

    private:

        Coord x_ = 0;
        Coord y_ = 0;
        uint8_t paletteOffset_ = 0;

    }; // rckid::Sprite

} // namespace rckid