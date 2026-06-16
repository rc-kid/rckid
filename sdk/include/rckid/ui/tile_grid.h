#pragma once

#include <rckid/graphics/tile_grid.h>
#include <rckid/ui/wrapper.h>

namespace rckid::ui {


    class TileGrid : public Wrapper<rckid::TileGrid> {
    public:

        static constexpr Coord tileWidth() { return rckid::TileGrid::Tile::width(); };
        static constexpr Coord tileHeight() { return rckid::TileGrid::Tile::height(); };

        TileGrid(Coord cols, Coord rows, Color::RGB565 const * palette):
            Wrapper{rckid::TileGrid{cols, rows, palette}}
        {

        }
    };

} // namespace rckid::ui
