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

    /** 8x16 tile with 16 colors used extensively in the UI for font rendering and symbols.
     
        Two pixels are packed in a single byte. 
     */
    template<>
    inline constexpr Tile<8, 16, Color::Index16>::Tile(uint8_t const (&c)[128]):
    #define C(A, B) static_cast<uint8_t>((c[B] << 4) | c[A]) 
        pixels_{
            C(7,15), C(23,31), C(39,47), C(55,63), C(71,79), C(87,95),  C(103,111), C(119,127),
            C(6,14), C(22,30), C(38,46), C(54,62), C(70,78), C(86,94),  C(102,110), C(118,126),
            C(5,13), C(21,29), C(37,45), C(53,61), C(69,77), C(85,93),  C(101,109), C(117,125),
            C(4,12), C(20,28), C(36,44), C(52,60), C(68,76), C(84,92),  C(100,108), C(116,124),
            C(3,11), C(19,27), C(35,43), C(51,59), C(67,75), C(83,91),   C(99,107), C(115,123),
            C(2,10), C(18,26), C(34,42), C(50,58), C(66,74), C(82,90),   C(98,106), C(114,122),
            C(1, 9), C(17,25), C(33,41), C(49,57), C(65,73), C(81,89),   C(97,105), C(113,121),
            C(0, 8), C(16,24), C(32,40), C(48,56), C(64,72), C(80,88),   C(96,104), C(112,120),
        } {
    #undef C
    }

} // namespace rckid