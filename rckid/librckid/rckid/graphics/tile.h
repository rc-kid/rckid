#pragma once

#include "rckid/rckid.h"
#include "Color.h"
#include "drawing.h"

namespace rckid {

    template<unsigned WIDTH, unsigned HEIGHT, PixelFormat FMT>
    class Tile {
    public:

        static constexpr unsigned Width = WIDTH;
        static constexpr unsigned Height = HEIGHT;
        static constexpr PixelFormat Format = FMT;

        static_assert(HEIGHT * bpp(FMT) % 32 == 0, "Tile column must fit in multiples of uint32_t values for performance reason");

        Tile() = default;

        /** Constexpr constructor from array of values.
         
            The array of values is expected to be in a row first mapping so that it can be easily hand edited in code, and will be converted to the appropriate column first bpp compacted format by the constructor. 
         */
        constexpr Tile(uint8_t const (& colors)[WIDTH * HEIGHT]);

        unsigned pixelAt(int x, int y) const { return rckid::pixelAt<FMT>(pixelBuffer_, WIDTH, HEIGHT, x, y); }
        void setPixelAt(int x, int y, unsigned c) { rckid::setPixelAt<FMT>(pixelBuffer_, WIDTH, HEIGHT, x, y, c); }

        void fill(uint16_t color) {
            rckid::fillBuffer<FMT>(pixelBuffer_, pixelBufferLength<FMT>(WIDTH, HEIGHT), color);
        }

        ColorRGB * renderColumn(int x, ColorRGB * buffer, ColorRGB const * palette = nullptr, uint8_t paletteOffset = 0) const {
            return reinterpret_cast<ColorRGB*>(convertToRGB<FMT>(
                getColumn<FMT>(pixelBuffer_, WIDTH, HEIGHT, x),
                reinterpret_cast<uint16_t*>(buffer), 
                HEIGHT, 
                palette,
                paletteOffset
            ));
        }

    private:

        uint32_t pixelBuffer_[pixelBufferLength<FMT>(WIDTH, HEIGHT)];

    }; 

    /** The simplest tile - 8x8 pixels with 256 colors. 
     */
    template<>
    inline constexpr Tile<8,8,PixelFormat::Color256>::Tile(uint8_t const (& c)[64]):
        pixelBuffer_{
            pack8(c[7],c[15],c[23],c[31]), pack8(c[39],c[47],c[55],c[63]),
            pack8(c[6],c[14],c[22],c[30]), pack8(c[38],c[46],c[54],c[62]),
            pack8(c[5],c[13],c[21],c[29]), pack8(c[37],c[45],c[53],c[61]),
            pack8(c[4],c[12],c[20],c[28]), pack8(c[36],c[44],c[52],c[60]),
            pack8(c[3],c[11],c[19],c[27]), pack8(c[35],c[43],c[51],c[59]),
            pack8(c[2],c[10],c[18],c[26]), pack8(c[34],c[42],c[50],c[58]),
            pack8(c[1], c[9],c[17],c[25]), pack8(c[33],c[41],c[49],c[57]),
            pack8(c[0], c[8],c[16],c[24]), pack8(c[32],c[40],c[48],c[56])
        } {
    }

    template<>
    inline constexpr Tile<8,8,PixelFormat::Color16>::Tile(uint8_t const (& c)[64]):
        pixelBuffer_{
            pack4(c[7],c[15],c[23],c[31],c[39],c[47],c[55],c[63]),
            pack4(c[6],c[14],c[22],c[30],c[38],c[46],c[54],c[62]),
            pack4(c[5],c[13],c[21],c[29],c[37],c[45],c[53],c[61]),
            pack4(c[4],c[12],c[20],c[28],c[36],c[44],c[52],c[60]),
            pack4(c[3],c[11],c[19],c[27],c[35],c[43],c[51],c[59]),
            pack4(c[2],c[10],c[18],c[26],c[34],c[42],c[50],c[58]),
            pack4(c[1], c[9],c[17],c[25],c[33],c[41],c[49],c[57]),
            pack4(c[0], c[8],c[16],c[24],c[32],c[40],c[48],c[56])
        } {
    }

    template<>
    inline constexpr Tile<12, 24, PixelFormat::Color16>::Tile(uint8_t const (& c)[12 * 24]):
    #define AT(R, C) c[(C) + (R) * 12]
    #define PACK(C, RS) pack4(AT(RS, C), AT(RS + 1, C), AT(RS + 2, C), AT(RS + 3, C), AT(RS + 4, C), AT(RS + 5, C), AT(RS + 6, C), AT(RS + 7, C))
    #define COL(C) PACK(C, 0), PACK(C, 8), PACK(C, 16)
        pixelBuffer_{
            COL(11), COL(10), COL(9), COL(8), 
            COL(7), COL(6), COL(5), COL(4), 
            COL(3), COL(2), COL(1), COL(0)
        } {
    #undef COL
    #undef PACK
    #undef AT
    }

} // namespace rckid