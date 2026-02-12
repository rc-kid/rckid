#pragma once

#include <rckid/graphics/tile_grid.h>
#include <rckid/ui/wrapper.h>

namespace rckid::ui {


    class TileGrid : public Wrapper<rckid::TileGrid> {
    public:

        TileGrid(Coord cols, Coord rows):
            Wrapper{rckid::TileGrid{cols, rows}}
        {

        }

        

        void renderColumn(Coord column, Coord starty, Color::RGB565 * buffer, Coord numPixels) override {
            contents_.renderColumn(column, starty, numPixels, buffer);
        }

    };

} // namespace rckid::ui
