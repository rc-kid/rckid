#pragma once

#include <rckid/game/asset.h>

namespace rckid::game {

    class Palette : public Asset {
    public:

        Color::RGB565 const * colors() const {
            UNIMPLEMENTED;
        }

        Color::RGB565 operator[](Color::Index256 index) const { return colors()[index.index()]; }

    }; // rckid::game::PaletteAsset

}