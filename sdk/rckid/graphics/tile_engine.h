#pragma once

#include "tile.h"
#include "sprite.h"
#include "renderer.h"

namespace rckid {

    /** Very simple single-layer tile engine with minimal memory footprint. 
     
        Mostly useful for UI applications. 
     */
    template<typename TILE>
    class SimpleTileEngine {
    public:
        using Tile = TILE;
        using Color = TILE::Color;

    }; // rckid::SimpleTileEngine

} // namespace rckid