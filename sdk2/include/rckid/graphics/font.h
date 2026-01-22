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


    using Font = immutable_ptr<FontData>;



} // namespace rckid