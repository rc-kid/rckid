#pragma once

#include <rckid/memory.h>
#include <rckid/string.h>

namespace rckid {

    /** Information about a single glyph. 
     
        Each character in a rendered text corresponds to a glyph that defines the look of that particular character. Each glyph consists of pixel data and information on where to render it (width, height, x and y offsets relative to the top left corner of the character cell) as well as the overall character width (advanceX). 

        Pixel data consists of tightly packed 2bpp (4 colors) values. To simplify rendering, each column of pixels aligns perfectly to 4 pixels (one byte)
     */
    class GlyphInfo {
    public:
        uint16_t const index;
        uint8_t const advanceX;
        int8_t const x;
        int8_t const y;
        uint8_t const width;
        uint8_t const height;

        constexpr GlyphInfo(uint16_t index, uint8_t advanceX, int8_t x, int8_t y, uint8_t width, uint8_t height):
            index{index}, advanceX{advanceX}, x{x}, y{y}, width{width}, height{height} {}

    private:

        uint8_t const _padding = 0; // to ensure that size is 8

    } __attribute__((packed)); // rckid::GlyphInfo

    static_assert(sizeof(GlyphInfo) == 8);

   
    /** Font data
     
        Class that holds the font information, including its size (height advance), information about the available glyphs and their pixel data. The class also defined common menthods on fonts, such as rendering and calculating text height. 
        
        The font itself (Font class) is just a pointer to the FontData. This is because fonts are usually immutable data in flash memory, making the immutable_ptr cheap to use and trivial to construct.

        NOTE that in order for the FontData to be able to stay in flash memory (rodata section), it cannot keep any smart pointers to its data. Therefore glyphs and pixels of the font *must* managed manually for dynamic fonts.
     */
    class FontData {
    public:
        Coord const size;
        uint32_t const numGlyphs;

        GlyphInfo const * const glyphs;

        uint8_t const * const pixels;

        constexpr FontData(int size, uint32_t numGlyphs, GlyphInfo const * glyphs, uint8_t const * pixels):
            size{static_cast<Coord>(size)}, numGlyphs{numGlyphs}, glyphs{glyphs}, pixels{pixels} {}

        Coord textWidth(char const * str) const {
            Coord result = 0;
            while(*str != 0)
                result += glyphs[static_cast<unsigned>(*str++) - 32].advanceX;
            return result;
        }

        Coord textWidth(String const & str) const { return textWidth(str.c_str()); }

        /** Returns glyph for given character. 
         
            The character is expected to be in the printable range, i.e numGlyphs characters from index 32, inclusive. If outside of the range, glyph infor for a question mark is returned instead. 
         */
        GlyphInfo const * glyphInfoFor(char glyph) const { 
            if (glyph < 32)
                return & glyphs['?' - 32];
            glyph -= 32;
            if (static_cast<uint32_t>(glyph) >= numGlyphs)
                return & glyphs['?' - 32];
            return & glyphs[static_cast<uint32_t>(glyph)]; 
        }

        /** Renders column of given glyph. 
         
            This corresponds to the renderColumn function common in other ui elements. The coordinates (column and startRow) are expected to be relative to the top left corner of the glyph bounding box (i.e. not the glyph data itself). 
         */
        void renderColumn(Coord column, Coord startRow, Coord numPixels, GlyphInfo const * gi,  Color::RGB565 * buffer, Color::RGB565 const * palette) const {
            // first updte the coordinates from the bounding box to the glyph space
            column -= gi->x;
            startRow -= gi->y;
            if (startRow < 0) {
                buffer += -startRow;
                numPixels += startRow; // startRow is negative
                startRow = 0;
            }
            // since we are not rendering background, check if there is anything to render first
            if (column < 0 || column >= gi->width)
                return;
            if (numPixels <= 0 || startRow >= gi->height)
                return;
            // move to current glyph
            uint8_t const * glyphPixels = pixels + gi->index;
            // move to current column
            int colHeight = gi->height;
            if (colHeight % 4 != 0)
                colHeight += 4 - (colHeight % 4);
            glyphPixels += column * colHeight / 4;
            // draw, 
            Coord y = 0;
            uint32_t bits = 0;
            uint32_t val = 0;
            Coord maxY = std::min(static_cast<Coord>(gi->height), numPixels);
            while (y < maxY) {
                if (bits == 0) {
                    val = *glyphPixels++;
                    bits = 8;
                }
                unsigned a = (val >> 6) & 0x3;
                if (y >= startRow) {
                    //*buffer = 0xff00;
                    if (a != 0)
                        *buffer = palette[a];
                    ++buffer;
                }
                val <<= 2;
                bits -= 2;
                ++y;
                --numPixels;
            }
        }

        void createFontPalette(Color::RGB565 * palette, Color fg) const {
            palette[0] = fg.withBrightness(0).toRGB565();
            palette[1] = fg.withBrightness(85).toRGB565();
            palette[2] = fg.withBrightness(170).toRGB565();
            palette[3] = fg.toRGB565();
        }
    }; 

    /** Font is a simple non-owning wrapper over FontData.
        
        This allows font itself to be extremely lightweight object (single pointer) and can be used with either flash based fonts, or in theory with dynamic fonts as well. 

        See the FontData class above for more information.
     */
    class Font {
    public:
        constexpr Font(FontData const * f): f_{f} {}
        constexpr Font(FontData const & f): f_{& f} {}
        constexpr Font(Font const & other): f_{other.f_} {}

        Font & operator = (Font const & other) {
            if (this != & other)
                f_ = other.f_;
            return *this;
        }

        FontData const * operator -> () const { return f_; }

        bool operator == (Font const & other) const { return f_ == other.f_; }
        bool operator != (Font const & other) const { return f_ != other.f_; }


    private:
        FontData const * f_;
    }; // rckid::Font

} // namespace rckid