#pragma once

#include "geometry.h"
#include "pixel_surface.h"
#include "color.h"

namespace rckid {

    /** Single tile. 
     
        Tile is a surface with statically known width and height. It supports the basic  
     */
    template<Coord WIDTH, Coord HEIGHT, typename PIXEL>
    class Tile {
    public:
        using Surface = PixelSurface<PIXEL::BPP>;
        using Pixel = PIXEL;
        static constexpr uint32_t BPP = PIXEL::BPP;

        static_assert((HEIGHT * BPP) % 32 == 0); // Tile column must fut multiples of uint32_t values for performance reasons

        /** Constexpr constructor from array of values.
         
            The array of values is expected to be in a row first mapping so that it can be easily hand edited in code, and will be converted to the appropriate column first bpp compacted format by the constructor. 
         */
        constexpr Tile(PIXEL const (& colors)[WIDTH * HEIGHT]);

        static constexpr Coord width() { return WIDTH; }
        static constexpr Coord height() { return HEIGHT; }

        // palette is not nullptr as we expect most tiles to use palette indices instead of RGB colors
        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, uint16_t * buffer, uint16_t const * palette) const {
            return renderColumn(pixels_, column, startRow, numPixels, WIDTH, HEIGHT, buffer, palette);
        }

        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, uint16_t * buffer, uint32_t transparent, uint16_t const * palette) const {
            return Surface::renderColumn(pixels_, column, startRow, numPixels, WIDTH, HEIGHT, buffer, transparent, palette);
        }

    private:

        uint16_t pixels_[Surface::numHalfWords(WIDTH, HEIGHT)];
    }; 

    /** The simplest tile - 8x8 pixels with 256 colors. 
     */
    template<>
    inline constexpr Tile<8, 8, Color256>::Tile(Color256 const (& c)[64]):
    #define C(A, B) static_cast<uint16_t>((c[B].toRaw() << 8) | c[A].toRaw()) 
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
    #define C(A, B) static_cast<uint16_t>((c[B].toRaw() << 8) | c[A].toRaw()) 
        pixels_{ 
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

    /** 8x16 tile with 16 colors only (ideal for Iosevka, used in the ui extensively (header, text boxes, etc.)
     */
    template<>
    inline constexpr Tile<8, 16, Color16>::Tile(Color16 const (& c)[128]):
    #define C(A, B, C, D) static_cast<uint16_t>((c[D].toRaw() << 12) | (c[C].toRaw() << 8) | (c[B].toRaw() << 4) | c[A].toRaw()) 
        pixels_{ 
            C(7,15,23,31), C(39,47,55,63), C(71,79,87,95),  C(103,111,119,127),
            C(6,14,22,30), C(38,46,54,62), C(70,78,86,94),  C(102,110,118,126),
            C(5,13,21,29), C(37,45,53,61), C(69,77,85,93),  C(101,109,117,125),
            C(4,12,20,28), C(36,44,52,60), C(68,76,84,92),  C(100,108,116,124),
            C(3,11,19,27), C(35,43,51,59), C(67,75,83,91),   C(99,107,115,123),
            C(2,10,18,26), C(34,42,50,58), C(66,74,82,90),   C(98,106,114,122),
            C(1, 9,17,25), C(33,41,49,57), C(65,73,81,89),   C(97,105,113,121),
            C(0, 8,16,24), C(32,40,48,56), C(64,72,80,88),   C(96,104,112,120),
        } {
    #undef C
    }

    /** The UI font tile with specific 12x24 size and 256 colors. 
     */
    template<>
    inline constexpr Tile<12, 24, Color256>::Tile(Color256 const (& c)[12 * 24]):
    #define AT(R, C) c[(C) + (R) * 12]
    #define PACK2(a,b) static_cast<uint16_t>((b.toRaw() << 8) | a.toRaw())
    #define PACK(C, RS) PACK2(AT(RS, C), AT(RS + 1, C)), PACK2(AT(RS + 2, C), AT(RS + 3, C)), PACK2(AT(RS + 4, C), AT(RS + 5, C)), PACK2(AT(RS + 6, C), AT(RS + 7, C))
    #define COL(C) PACK(C, 0), PACK(C, 8), PACK(C, 16)
        pixels_{
            COL(11), COL(10), COL(9), COL(8), 
            COL(7), COL(6), COL(5), COL(4), 
            COL(3), COL(2), COL(1), COL(0)
        } {
    #undef COL
    #undef PACK
    #undef AT
    }

    /** The UI font tile with specific 12x24 size and 16 colors. 
     */
    template<>
    inline constexpr Tile<12, 24, Color16>::Tile(Color16 const (& c)[12 * 24]):
    #define AT(R, C) c[(C) + (R) * 12]
    #define PACK4(a,b,c, d) static_cast<uint16_t>((d.toRaw() << 12) | (c.toRaw() << 8) | (b.toRaw() << 4) | a.toRaw())
    #define PACK(C, RS) PACK4(AT(RS, C), AT(RS + 1, C), AT(RS + 2, C), AT(RS + 3, C)), PACK4(AT(RS + 4, C), AT(RS + 5, C), AT(RS + 6, C), AT(RS + 7, C))
    #define COL(C) PACK(C, 0), PACK(C, 8), PACK(C, 16)
        pixels_{
            COL(11), COL(10), COL(9), COL(8), 
            COL(7), COL(6), COL(5), COL(4), 
            COL(3), COL(2), COL(1), COL(0)
        } {
    #undef COL
    #undef PACK
    #undef AT
    }



} // namespace rckid