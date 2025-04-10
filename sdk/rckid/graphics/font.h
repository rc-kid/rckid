#pragma once

#include <string>
#include <array>

#include <platform.h>
#include "geometry.h"
#include "color.h"

namespace rckid {

    /** Information about single glyph. 
     */
    PACKED(class GlyphInfo {
    public:
        uint16_t index;
        uint8_t advanceX;
        int8_t x;
        int8_t y;
        uint8_t width;
        uint8_t height;

        constexpr GlyphInfo(uint16_t index, uint8_t advanceX, int8_t x, int8_t y, uint8_t width, uint8_t height):
            index{index}, advanceX{advanceX}, x{x}, y{y}, width{width}, height{height} {}
    private:
        uint8_t _padding = 0;

    }); // rckid::GlyphInfo

    static_assert(sizeof(GlyphInfo) == 8);

    class Font {
    public:
        int size; 
        int numGlyphs;
        GlyphInfo const * glyphs;
        uint8_t const * pixels;

        template<typename T>
        static constexpr Font const fromROM() {
            return Font {
                T::size, 
                sizeof(T::glyphs) / sizeof(GlyphInfo),
                T::glyphs, 
                T::pixels
            };
        }

        int textWidth(char const * str) const {
            int result = 0;
            while(*str != 0)
                result += glyphs[static_cast<unsigned>(*str++) - 32].advanceX;
            return result;
        }

        int textWidth(String const & str) const { return textWidth(str.c_str()); }

        GlyphInfo const & glyphInfoFor(char glyph) const { return glyphs[glyph - 32]; }

        /** Takes a color and converts it to an array of 4 colors corresponding to the 2BPP of font glyphs. 
         
            This is the default implementation that works well for palette based colors, where the palette is created from preceding color indices. Non-palette colors are implemented in specializations below. 
         */
        template<typename COLOR>
        static std::array<uint16_t, 4> colorToArray(COLOR color) {
            uint16_t i = color.raw16();
            return std::array<uint16_t, 4>{
                static_cast<uint16_t>((i - 3) % (1 << COLOR::BPP)),
                static_cast<uint16_t>((i - 2) % (1 << COLOR::BPP)),
                static_cast<uint16_t>((i - 1) % (1 << COLOR::BPP)),
                i, 
            };
        }

        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels, GlyphInfo const * gi, uint16_t * palette) {
            // move to current glyph
            uint8_t const * glyphPixels = pixels + gi->index;
            // move to current column
            int colHeight = gi->height;
            if (colHeight % 4 != 0)
                colHeight += 4 - (colHeight % 4);
            glyphPixels += column * colHeight / 4;
            // draw 
            int y = 0;
            uint32_t bits = 0;
            uint32_t val = 0;
            while (numPixels > 0) {
                if (bits == 0) {
                    val = *glyphPixels++;
                    bits = 8;
                }
                unsigned a = (val >> 6) & 0x3;
                if (y >= starty) {
                    //*buffer = 0xff00;
                        if (a != 0)
                        *buffer = palette[a];
                    ++buffer;
                    --numPixels;
                }
                val <<= 2;
                bits -= 2;
                ++y;
            }
        }

    }; // rckid::Font


    /** Font color array specialization for RGB565, full color is the specified, others are with decreasing alpha to black.
     */
    /*
    template<>
    inline std::array<uint16_t, 4> Font::colorToArray(ColorRGB565 color) {
        return std::array<uint16_t, 4>{
            0,
            color.withAlpha(85).raw(),
            color.withAlpha(170).raw(),
            color.raw()
        };
    }
        */

} // namespace rckid