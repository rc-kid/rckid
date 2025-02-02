#pragma once

#include "color.h"

namespace rckid {

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

} // namespace rckid