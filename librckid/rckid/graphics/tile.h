#pragma once

#include "rckid/rckid.h"

namespace rckid {

    /** Single Tile

        Tiles are 16x16 pixels and use 256 colors from a custom palette with the color 0 being transparent 
     */
    class Tile {
    public:

        uint8_t at(int x, int y) const { return data_[y + x * 16]; }
        uint8_t & at(int x, int y) { return data_[y + x * 16]; }

    private:

        uint8_t data_[16 * 16];

    }; // rckid::Tile

} // namespace rckid