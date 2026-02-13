#pragma once

#include <rckid/ui/tile_grid.h>

namespace rckid::ui {

    class Header : public TileGrid {
    public:

        Header():
            TileGrid{display::WIDTH / TileGrid::tileWidth(), 1, defaultPalette()}
        {
            setRect(Rect::WH(display::WIDTH, TileGrid::tileHeight()));
            contents_.text(0,0) << "RCKid SDK 1.0";
        }

        static mutable_ptr<Color::RGB565> defaultPalette() {
            mutable_ptr<Color::RGB565> result{new Color::RGB565[32], sizeof(Color::RGB565) * 32};
            Color textFg = Style::defaultStyle()->defaultFg();
            for (uint8_t i = 0; i < 16; ++i)
                result.mut()[i] = textFg.withBrightness(i << 4 | i).toRGB565();
            return result;
        }
    


    }; // rckid::ui::Header

} // namespace rckid::ui