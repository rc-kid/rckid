#pragma once

#include "color.h"

namespace rckid {

    using Palette = ColorRGB const *;

    template<typename COLOR>
    class PaletteHolder {
    public:
        ColorRGB const * palette() const {
            return palette_;
        }

        void setPalette(ColorRGB const * value) {
            palette_ = value;
        }

    protected:
        PaletteHolder() = default;
        PaletteHolder(ColorRGB const * palette): palette_{palette} {}

        ColorRGB const * palette_ = nullptr;
    };

    template<>
    class PaletteHolder<ColorRGB565> {};

    template<>
    class PaletteHolder<ColorRGB332> {};


    template<typename COLOR>
    class PaletteOffsetHolder {
    public:
        uint8_t paletteOffset() const { return paletteOffset_; }
        void setPaletteOffset(uint8_t offset) { paletteOffset_ = offset; }

    protected:
        PaletteOffsetHolder() = default;
        PaletteOffsetHolder(uint8_t offset): paletteOffset_{offset} {}

        uint8_t paletteOffset_ = 0;
    }; 

    template<>
    class PaletteOffsetHolder<ColorRGB565> {};

    template<>
    class PaletteOffsetHolder<ColorRGB332> {};


} // namespace rckid