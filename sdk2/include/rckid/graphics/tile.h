#pragma once

#include <rckid/graphics/geometry.h>
#include <rckid/graphics/color.h>
#include <rckid/graphics/blit.h>

namespace rckid {

    /** Tile
     * 
     */
    template<Coord WIDTH, Coord HEIGHT, typename PIXEL>
    class Tile {
    public:

        static_assert(PIXEL::Indexed); 

        static constexpr Coord width() { return WIDTH; }
        static constexpr Coord height() { return HEIGHT; }

        /** Constexpr constructor from array of values.
         
            The array of values is expected to be in a row first mapping so that it can be easily hand edited in code, and will be converted to the appropriate column first bpp compacted format by the constructor. 
         */
        constexpr Tile(uint8_t const (& colors)[WIDTH * HEIGHT]);


        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, Color::RGB565 * buffer, Color::RGB565 const * palette) const {
            UNIMPLEMENTED;
        }

        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, Color::RGB565 * buffer, Color::RGB565 const * palette, uint32_t transparentColor) const {
            UNIMPLEMENTED;
        }

    private:

        // the pixel array
        uint8_t pixels_[PIXEL::getPixelArraySize(WIDTH, HEIGHT)];

    }; // rckid::Tile

    /** The simplest tile - 8x8 pixels with 256 colors. 
     
        Here we just need to convert the row first array of color indices to column first array of bytes.
     */
    template<>
    inline constexpr Tile<8, 8, Color::Index256>::Tile(uint8_t const (& c)[64]):
        pixels_{
            c[7], c[15], c[23], c[31], c[39], c[47], c[55], c[63],
            c[6], c[14], c[22], c[30], c[38], c[46], c[54], c[62],
            c[5], c[13], c[21], c[29], c[37], c[45], c[53], c[61],
            c[4], c[12], c[20], c[28], c[36], c[44], c[52], c[60],
            c[3], c[11], c[19], c[27], c[35], c[43], c[51], c[59],
            c[2], c[10], c[18], c[26], c[34], c[42], c[50], c[58],
            c[1], c[9],  c[17], c[25], c[33], c[41], c[49], c[57],
            c[0], c[8],  c[16], c[24], c[32], c[40], c[48], c[56],  
        } {
    }

} // namespace rckid