#pragma once

#include "rckid/graphics/tile_engine.h"
#include "assets/tiles/Iosevka_Tiles_12x24.h"

namespace rckid {

    using UITile = Tile<12, 24, PixelFormat::Color16>;

    class UITileEngine : public SimpleEngine<UITile> {
    public:

        using Color = ColorRGB_332;

        UITileEngine(int width, int height):
            SimpleEngine{width, height} {
            setTiles(Iosevka_Tiles_12x24::tileset);    
            top_ = 235 - pixelHeight();
        }

    }; 


}