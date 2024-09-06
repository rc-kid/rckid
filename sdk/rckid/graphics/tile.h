#pragma once

#include "geometry.h"
#include "color.h"
#include "drawing.h"

namespace rckid {

    /** Single tile implementation. 
     
        Tiles are wrappers around pixel buffers of statically known sizes (and as such they share the pixel buffer's drawing capabilities similar to bitmaps). For performance reasons, no matter what the color depth used for the tile, each column of the tile must perfectly fit into 32bit reads, i.e. with 4 bpp tile heights that are multiples of 8 are possible, for 8 bpp multiples of 4 and finally for the RGB (16bpp) tiles multiples of 2 are necessary. 

        As a convenience, tiles have constructors that can be called with naturally (i.e. left to right row first) ordered pixels that will be converted to the native pixel order (column first, right to left). 
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
        constexpr Tile(uint8_t const (& colors)[WIDTH * HEIGHT]);

        COLOR pixelAt(Coord x, Coord y) const { 
            return pixelBufferAt<COLOR>(pixelBuffer_, x, y, WIDTH, HEIGHT); 
        }
        
        void setPixelAt(Coord x, Coord y, COLOR c) { 
            setPixelBufferAt<COLOR>(pixelBuffer_, x, y, c, WIDTH, HEIGHT); 
        }

        void fill(COLOR color) {
            pixelBufferFill(pixelBuffer_, WIDTH * HEIGHT, color);
        }

        // TODO ideally this should be private somehow
        ColorRGB * renderColumn(Coord x, ColorRGB * buffer, ColorRGB const * palette = nullptr, uint8_t paletteOffset = 0) const {
            return pixelBufferToRGB<COLOR>(
                pixelBuffer_ + pixelBufferColumnOffset<COLOR>(WIDTH, HEIGHT, x),
                buffer, 
                HEIGHT, 
                palette,
                paletteOffset
            );
        }

    private:

        uint8_t pixelBuffer_[pixelBufferSize<COLOR>(WIDTH, HEIGHT)];

    }; // rckid::Tile<W,H,COLOR>



    /** The simplest tile - 8x8 pixels with 256 colors. 
     */
    template<>
    inline constexpr Tile<8, 8, Color256>::Tile(uint8_t const (& c)[64]):
        pixelBuffer_{
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

    /** 8x8 tile with only 16 colors, that occupies only 32 bytes in memory.
     */
    template<>
    inline constexpr Tile<8, 8, Color16>::Tile(uint8_t const (& c)[64]):
    #define PACK2(a,b) static_cast<uint8_t>(static_cast<uint8_t>(b << 4) | a)
        pixelBuffer_{
            PACK2(c[7],c[15]), PACK2(c[23],c[31]), PACK2(c[39],c[47]), PACK2(c[55],c[63]),
            PACK2(c[6],c[14]), PACK2(c[22],c[30]), PACK2(c[38],c[46]), PACK2(c[54],c[62]),
            PACK2(c[5],c[13]), PACK2(c[21],c[29]), PACK2(c[37],c[45]), PACK2(c[53],c[61]),
            PACK2(c[4],c[12]), PACK2(c[20],c[28]), PACK2(c[36],c[44]), PACK2(c[52],c[60]),
            PACK2(c[3],c[11]), PACK2(c[19],c[27]), PACK2(c[35],c[43]), PACK2(c[51],c[59]),
            PACK2(c[2],c[10]), PACK2(c[18],c[26]), PACK2(c[34],c[42]), PACK2(c[50],c[58]),
            PACK2(c[1], c[9]), PACK2(c[17],c[25]), PACK2(c[33],c[41]), PACK2(c[49],c[57]),
            PACK2(c[0], c[8]), PACK2(c[16],c[24]), PACK2(c[32],c[40]), PACK2(c[48],c[56]),
        } {
    #undef PACK2
    }

    /** The UI font tile with specific 12x24 size and 16 colors. 
     */
    template<>
    inline constexpr Tile<12, 24, Color16>::Tile(uint8_t const (& c)[12 * 24]):
    #define AT(R, C) c[(C) + (R) * 12]
    #define PACK2(a,b) static_cast<uint8_t>(static_cast<uint8_t>(b << 4) | a)
    #define PACK(C, RS) PACK2(AT(RS, C), AT(RS + 1, C)), PACK2(AT(RS + 2, C), AT(RS + 3, C)), PACK2(AT(RS + 4, C), AT(RS + 5, C)), PACK2(AT(RS + 6, C), AT(RS + 7, C))
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