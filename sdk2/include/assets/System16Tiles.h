#pragma once

#include <rckid/graphics/tile.h>

namespace rckid::assets {

    /* Manually crafted system font tiles.

       Filename:       fonts/Iosevka.ttf
       Font size:      16
       Tile width:     8
       Tile height:    16
       Tile bpp:       4
       tiles:          95
     */
    static constexpr Tile<8,16,Color::Index16> System16Tiles[] = {
        #define __ 0
        // 0: ' ', codepoint 32, utf8: ` `
        Tile<8,16,Color::Index16>{{
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
            __, __, __, __, __, __, __, __, 
        }},

        #undef __
    };
} // namespace rckid::assets
