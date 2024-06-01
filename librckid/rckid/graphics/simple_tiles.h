#pragma once

#include "tile.h"

namespace rckid {

    /** Very simple tile engine for UI elements with minimal memory footprint. 
     
        Utilizes a single tilemap with no hidden areas and tilemap with palette offsets for different color schemes. Mainly useful for UI overlays, where the sprites and tiles are all stored in ROM, but will work from any memory location. 
     */
    class SimpleTiles {
    public:

        class TileInfo {
        public:
            char c;
            uint8_t offset = 0;

            TileInfo & operator = (char ch) { c = ch; return *this; }

        } __attribute__((packed)); 



    private:



        int w_;
        int h_;


    }; // rckid::SimpleTiles 


} // namespace rckid