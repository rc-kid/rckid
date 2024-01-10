#pragma once 

#include "rckid/rckid.h"

namespace rckid {

    /** Tiling engine renderer
     */
    template<typename COLOR, size_t W, size_t H>
    class TileEngine {
    public:
        using Color = COLOR;
        
        class Tile {
        public:
            Color buffer[W * H];
        }; 


    }; // rckid::TileEngine

} // namespace rckid