#pragma once

#include <kiddo/graphics/tile_grid.h>
#include <kiddo/ui/wrappers.h>

namespace kiddo::ui {


    class TileGrid : public Wrapper<kiddo::TileGrid> {
    public:

        static constexpr Coord tileWidth() { return kiddo::TileGrid::Tile::width(); };
        static constexpr Coord tileHeight() { return kiddo::TileGrid::Tile::height(); };

        TileGrid(Coord cols, Coord rows, Color::RGB565 const * palette):
            Wrapper{kiddo::TileGrid{cols, rows, palette}}
        {

        }
    };

} // namespace kiddo::ui
