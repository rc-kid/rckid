#pragma once

#include "rckid/rckid.h"

namespace rckid {

    /* 
    
    tile = just the raw data and extractors for x & y 
    sprite = tile + tileinfo 
    
    */

    template<int WIDTH, int HEIGHT, uint8_t BPP>
    class Tile {
    public:

        static_assert(BPP == 2 || BPP == 4 || BPP == 8, "Only 2, 4 or 8 bits per pixel are allowed");
        static_assert(HEIGHT * BPP % 32 == 0, "Tile column must fit in uint32_t values for performance reasons");

        /** Default constructor for a clear tile 
         */
        Tile() = default;
        /** And default copy constructor. 
         */
        Tile(Tile const & from) = default;

        /** Constexpr constructor from array of values.
         
            The array of values is expected to be in a row first mapping so that it can be easily hand edited in code, and will be converted to the appropriate column first bpp compacted format by the constructor. 
         */
        constexpr Tile(uint8_t const (& colors)[WIDTH * HEIGHT]);

        int width() const { return WIDTH; }
        int height() const { return HEIGHT; }
        uint8_t bpp() const { return BPP; }

        uint8_t at(int x, int y) const {
            switch (BPP) {
                case 8:
                    return getColumn(x)[y];
                case 4:
                case 2:
                    UNIMPLEMENTED;
                default:
                    UNREACHABLE;
            }
        }

        void setAt(int x, int y, uint8_t c) {
            switch (BPP) {
                case 8:
                    getColumn(x)[y] = c;
                    break;
                case 4:
                case 2:
                    UNIMPLEMENTED;
                default:
                    UNREACHABLE;
            }
        }

        uint8_t const * getColumn(int x) const {
            return rawData_ + (HEIGHT * x * BPP / 8);
        }

        uint8_t * getColumn(int x) {
            return rawData_ + (HEIGHT * x * BPP / 8);
        }

    private:
        uint8_t rawData_[WIDTH * HEIGHT * BPP / 8];
    }; // rckid::Tile


    /** The simplest tile - 8x8 pixels with 256 colors. 
     */
    template<>
    inline constexpr Tile<8,8,8>::Tile(uint8_t const (& c)[64]):
        rawData_{
            c[0],c[8],c[16],c[24],c[32],c[40],c[48],c[56],
            c[1],c[9],c[17],c[25],c[33],c[41],c[49],c[57],
            c[2],c[10],c[18],c[26],c[34],c[42],c[50],c[58],
            c[3],c[11],c[19],c[27],c[35],c[43],c[51],c[59],
            c[4],c[12],c[20],c[28],c[36],c[44],c[52],c[60],
            c[5],c[13],c[21],c[29],c[37],c[45],c[53],c[61],
            c[6],c[14],c[22],c[30],c[38],c[46],c[54],c[62],
            c[7],c[15],c[23],c[31],c[39],c[47],c[55],c[63]
        } {
    }

    /** Rather special tile for the UI  */
    /*
    template<>
    inline constexpr Tile<12,24,4>::Tile(uint8_t const (& c)[288]):
        rawData_{


        }{
    }
    */
} // namespace rckid