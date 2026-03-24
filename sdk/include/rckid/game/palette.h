#pragma once

#include <rckid/game/asset.h>

namespace rckid::game {

    class Palette : public Asset {
    public:

        char const * className() const override { return "Palette"; }

        Palette() {
            fillDefaultPalette();
        }

        Palette(String name):
            Asset{std::move(name)}
        {
            fillDefaultPalette();
        }

        Color::RGB565 const * colors() const { return colors_; }

        Color::RGB565 operator[](Color::Index256 index) const { return colors()[index.index()]; }


    private:

        void fillDefaultPalette() {
            for (uint32_t i = 0; i < 256; ++i)
                colors_[i] = Color{Color::RGB332{static_cast<uint16_t>(i)}}.toRGB565();
        }

        Color::RGB565 colors_[256];
    }; // rckid::game::PaletteAsset

}