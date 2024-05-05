#pragma once

#include "rckid/rckid.h"

namespace rckid {

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
        int bpp; 
        int padding;
        int numGlyphs;
        GlyphInfo const * glyphs;
        uint32_t const * pixels;

        template<typename T>
        static constexpr Font const fromROM() {
            return Font {
                T::size, 
                T::bpp, 
                T::padding, 
                sizeof(T::glyphs) / sizeof(GlyphInfo),
                T::glyphs, 
                T::pixels
            };
        }
    }; // rckid::Font

} // namespace rckid