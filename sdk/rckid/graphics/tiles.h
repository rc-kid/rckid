#pragma once

#include "geometry.h"
#include "pixel_array.h"

namespace rckid {

    /** Single tile implementation. 
 
        Tiles are wrappers around pixel arrays of statically known sizes. For performance reasons, no matter what the color depth used for the tile, each column of the tile must perfectly fit into 32bit reads, i.e. with 4 bpp tile heights that are multiples of 8 are possible, for 8 bpp multiples of 4 and finally for the RGB (16bpp) tiles multiples of 2 are necessary. 

        As a convenience, tiles have constructors that can be called with naturally (i.e. left to right row first) ordered pixels that will be converted to the native pixel order of the pixel array (column first, right to left). 
     */
    template<Coord WIDTH, Coord HEIGHT, typename COLOR>
    class Tile {
    public:
        static constexpr Coord Width = WIDTH;
        static constexpr Coord Height = HEIGHT;
        static constexpr uint32_t BPP = COLOR::BPP;
        using Color = COLOR;

        static_assert((HEIGHT * BPP) % 32 == 0); // Tile column must fut multiples of uint32_t values for performance reasons

        /** Constexpr constructor from array of values.
         
            The array of values is expected to be in a row first mapping so that it can be easily hand edited in code, and will be converted to the appropriate column first bpp compacted format by the constructor. 
         */
        constexpr Tile(uint8_t const (& colors)[WIDTH * HEIGHT]));


    private:

        uint8_t pixels_[PixelArray<BPP>::numBytes(WIDTH, HEIGHT)];

    }; // rckid::Tile

    class TileMap {

    }; // rckid::TileMap
    
} // namespace rckid