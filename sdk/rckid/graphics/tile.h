#pragma once

#include "geometry.h"
#include "surface.h"
#include "color.h"

namespace rckid {

    /** Single tile. 
     
        Tile is a surface with statically known width and height. It supports the basic  
     */
    template<Coord WIDTH, Coord HEIGHT, typename PIXEL>
    class Tile : protected Surface<PIXEL::BPP> {
    public:
        
        using Pixel = PIXEL;
        static constexpr uint32_t BPP = PIXEL::BPP;

        static_assert((HEIGHT * BPP) % 32 == 0); // Tile column must fut multiples of uint32_t values for performance reasons

        /** Constexpr constructor from array of values.
         
            The array of values is expected to be in a row first mapping so that it can be easily hand edited in code, and will be converted to the appropriate column first bpp compacted format by the constructor. 
         */
        constexpr Tile(PIXEL const (& colors)[WIDTH * HEIGHT]);

        Coord width() const { return WIDTH; }
        Coord height() const { return HEIGHT; }

        // palette is not nullptr as we expect most tiles to use palette indices instead of RGB colors
        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, uint16_t * buffer, uint16_t const * palette) const {
            return renderColumn(pixels_, column, startRow, numPixels, WIDTH, HEIGHT, buffer, palette);
        }

    private:
        using Surface<BPP>::numHalfWords;
        using Surface<BPP>::renderColumn;

        uint16_t pixels_[numHalfWords(WIDTH, HEIGHT)];
    }; 

    /** The simplest tile - 8x8 pixels with 256 colors. 
     */
    template<>
    inline constexpr Tile<8, 8, Color256>::Tile(Color256 const (& c)[64]):
    #define C(A, B) static_cast<uint16_t>((c[A].toRaw() << 8) | c[B].toRaw()) 
        pixels_{
            C(7,15),C(23,31), C(39,47),C(55,63),
            C(6,14),C(22,30), C(38,46),C(54,62),
            C(5,13),C(21,29), C(37,45),C(53,61),
            C(4,12),C(20,28), C(36,44),C(52,60),
            C(3,11),C(19,27), C(35,43),C(51,59),
            C(2,10),C(18,26), C(34,42),C(50,58),
            C(1, 9),C(17,25), C(33,41),C(49,57),
            C(0, 8),C(16,24), C(32,40),C(48,56),
        } {
    #undef C
    }

    /** 8x16 tile (ideal for Iosevka) 
     */
    template<>
    inline constexpr Tile<8, 16, Color256>::Tile(Color256 const (& c)[128]):
    #define C(A, B) static_cast<uint16_t>((c[A].toRaw() << 8) | c[B].toRaw()) 
        pixels_{ // 79, 87, 96, 103, 111, 119, 127 
            C(7,15),C(23,31), C(39,47),C(55,63), C(71,79),C(87,95),  C(103,111),C(119,127),
            C(6,14),C(22,30), C(38,46),C(54,62), C(70,78),C(86,94),  C(102,110),C(118,126),
            C(5,13),C(21,29), C(37,45),C(53,61), C(69,77),C(85,93),  C(101,109),C(117,125),
            C(4,12),C(20,28), C(36,44),C(52,60), C(68,76),C(84,92),  C(100,108),C(116,124),
            C(3,11),C(19,27), C(35,43),C(51,59), C(67,75),C(83,91),   C(99,107),C(115,123),
            C(2,10),C(18,26), C(34,42),C(50,58), C(66,74),C(82,90),   C(98,106),C(114,122),
            C(1, 9),C(17,25), C(33,41),C(49,57), C(65,73),C(81,89),   C(97,105),C(113,121),
            C(0, 8),C(16,24), C(32,40),C(48,56), C(64,72),C(80,88),   C(96,104),C(112,120),
        } {
    #undef C
    }
    

} // namespace rckid