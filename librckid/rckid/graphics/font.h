#pragma once

#include "rckid/rckid.h"

namespace rckid {

    namespace glyph {
#define GLYPH(ID, NAME, ...) static constexpr char NAME = ID + 32;
#include "symbols.inc"
    } // namespace rckid::glyph

    /** Information about single glyph. 
     */
    class GlyphInfo {
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

    } __attribute__((packed)); // rckid::GlyphInfo

    static_assert(sizeof(GlyphInfo) == 8);
 
    class Font {
    public:
        int size; 
        int numGlyphs;
        GlyphInfo const * glyphs;
        uint32_t const * pixels;

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

    }; // rckid::Font

} // namespace rckid