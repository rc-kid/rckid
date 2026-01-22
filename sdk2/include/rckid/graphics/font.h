#pragma once

#include <rckid/memory.h>

namespace rckid {

    class GlyphInfo {
    public:

    } __attribute__((packed)); // rckid::GlyphInfo

   
    class FontData {
        Coord const size;
        uint32_t const numGlyphs;

        immutable_ptr<GlyphInfo> glyphs;

        immutable_ptr<uint8_t> pixels;

    }; 

    /** Font is an immutable pointer to FontData structure.
        
        This allows font itself to be extremely lightweight object (single pointer) and can be used with either flash based fonts, or in theory with dynamic fonts as well. Once the font is used though, it must become immutable as font users usually cache glyph information for faster rendering.
     */
    using Font = immutable_ptr<FontData>;

} // namespace rckid