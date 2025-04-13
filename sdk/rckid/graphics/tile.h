#pragma once

#include "geometry.h"
#include "pixel_array.h"

namespace rckid {

    /** Single tile implementation. 
 
        Tiles are wrappers around pixel arrays of statically known sizes. For performance reasons, no matter what the color depth used for the tile, each column of the tile must perfectly fit into 32bit reads, i.e. with 4 bpp tile heights that are multiples of 8 are possible, for 8 bpp multiples of 4 and finally for the RGB (16bpp) tiles multiples of 2 are necessary. 

        As a convenience, tiles have constructors that can be called with naturally (i.e. left to right row first) ordered pixels that will be converted to the native pixel order of the pixel array (column first, right to left). 
     */
    template<Coord WIDTH, Coord HEIGHT, uint32_t BITS_PER_PIXEL>
    class Tile {
    public:
        static constexpr Coord Width = WIDTH;
        static constexpr Coord Height = HEIGHT;
        static constexpr uint32_t BPP = BITS_PER_PIXEL;

        using Pixel = std::conditional_t<BITS_PER_PIXEL == 16, uint16_t, uint8_t>;

        static_assert((HEIGHT * BPP) % 32 == 0); // Tile column must fut multiples of uint32_t values for performance reasons

        /** Constexpr constructor from array of values.
         
            The array of values is expected to be in a row first mapping so that it can be easily hand edited in code, and will be converted to the appropriate column first bpp compacted format by the constructor. 
         */
        constexpr Tile(Pixel const (& colors)[WIDTH * HEIGHT]);


    private:

        uint8_t pixels_[PixelArray<BITS_PER_PIXEL>::numBytes(WIDTH, HEIGHT)];

    }; // rckid::Tile

    /** The simplest tile - 8x8 pixels with 256 colors. 
     */
    template<>
    inline constexpr Tile<8, 8, 8>::Tile(uint8_t const (& c)[64]):
        pixels_{
            c[7],c[15],c[23],c[31], c[39],c[47],c[55],c[63],
            c[6],c[14],c[22],c[30], c[38],c[46],c[54],c[62],
            c[5],c[13],c[21],c[29], c[37],c[45],c[53],c[61],
            c[4],c[12],c[20],c[28], c[36],c[44],c[52],c[60],
            c[3],c[11],c[19],c[27], c[35],c[43],c[51],c[59],
            c[2],c[10],c[18],c[26], c[34],c[42],c[50],c[58],
            c[1], c[9],c[17],c[25], c[33],c[41],c[49],c[57],
            c[0], c[8],c[16],c[24], c[32],c[40],c[48],c[56],
        } {
    }

    /** 8x16 tile (ideal for Iosevka) 
     */
    template<>
    inline constexpr Tile<8, 16, 8>::Tile(uint8_t const (& c)[128]):
        pixels_{ // 79, 87, 96, 103, 111, 119, 127 
            c[7],c[15],c[23],c[31], c[39],c[47],c[55],c[63], c[71],c[79],c[87],c[95],  c[103],c[111],c[119],c[127],
            c[6],c[14],c[22],c[30], c[38],c[46],c[54],c[62], c[70],c[78],c[86],c[94],  c[102],c[110],c[118],c[126],
            c[5],c[13],c[21],c[29], c[37],c[45],c[53],c[61], c[69],c[77],c[85],c[93],  c[101],c[109],c[117],c[125],
            c[4],c[12],c[20],c[28], c[36],c[44],c[52],c[60], c[68],c[76],c[84],c[92],  c[100],c[108],c[116],c[124],
            c[3],c[11],c[19],c[27], c[35],c[43],c[51],c[59], c[67],c[75],c[83],c[91],   c[99],c[107],c[115],c[123],
            c[2],c[10],c[18],c[26], c[34],c[42],c[50],c[58], c[66],c[74],c[82],c[90],   c[98],c[106],c[114],c[122],
            c[1], c[9],c[17],c[25], c[33],c[41],c[49],c[57], c[65],c[73],c[81],c[89],   c[97],c[105],c[113],c[121],
            c[0], c[8],c[16],c[24], c[32],c[40],c[48],c[56], c[64],c[72],c[80],c[88],   c[96],c[104],c[112],c[120],
        } {
    }
    

} // namespace rckid