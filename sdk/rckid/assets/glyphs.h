#pragma once

namespace rckid::assets {

    enum class Glyphs : char {
        #define GLYPH(NAME, ...) NAME,
        #include "symbol-glyphs.inc.h"
    }; // rckid::Glyphs

    namespace glyph {
        #define GLYPH(NAME, ...) constexpr char NAME = static_cast<char>(Glyphs::NAME) + 32;
        #include "symbol-glyphs.inc.h"
    } // rckid::glyphs

} // namespace rckid