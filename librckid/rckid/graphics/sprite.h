#pragma once

#include "tile.h"

namespace rckid {

    template<int WIDTH, int HEIGHT, uint8_t BPP>
    class Sprite : public Tile<WIDTH, HEIGHT, BPP> {
    public:
        int x() const { return x_; }
        int y() const { return y_; }
        uint8_t paletteOffset() const { return paletteOffset_; }
        void setX(int x) { x_ = x; }
        void setY(int y) { y_ = y; }
        void setPaletteOffset(uint8_t offset) { paletteOffset_ = offset; }
    private:
        int x_ = 0;
        int y_ = 0; 
        uint8_t paletteOffset_ = 0;
    }; 

} // namespace rckid