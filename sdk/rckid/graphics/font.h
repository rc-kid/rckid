#pragma once

#include <string>
#include <array>

#include <platform.h>
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

        int textWidth(std::string const & str) const { return textWidth(str.c_str()); }

        GlyphInfo const & glyphInfoFor(char glyph) const { return glyphs[glyph - 32]; }

        /** Takes a color and converts it to an array of 4 colors corresponding to the 2BPP of font glyphs. 
         
            This is the default implementation that works well for palette based colors, where the palette is created from preceding color indices. Non-palette colors are implemented in specializations below. 
         */
        template<typename COLOR>
        static std::array<uint16_t, 4> colorToArray(COLOR color) {
            uint16_t i = color.raw();
            return std::array<uint16_t, 4>{
                static_cast<uint16_t>((i - 3) % (1 << COLOR::BPP)),
                static_cast<uint16_t>((i - 2) % (1 << COLOR::BPP)),
                static_cast<uint16_t>((i - 1) % (1 << COLOR::BPP)),
                i, 
            };
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