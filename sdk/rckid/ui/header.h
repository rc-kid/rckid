#pragma once

#include "widget.h"
#include "../graphics/tile.h"
#include "../assets/tiles/system16.h"

namespace rckid::ui {

    /** Application hedear. 
     
        The header displays information such as current time, battery status, sound volume, signal strength, etc.

        If we use UI tiles then the header proper is very simple to do both column and row wise and can be displayed over large variety of renderers. So go make tiles:)
     */
    class Header : public Widget {
    public:

    protected:
        /** Renders the header by column. 
         
            
         */
        void renderColumn(Coord column, Pixel * buffer, Coord stary, Coord numPixels) override {
            uint32_t tile = column / 8;
            uint32_t tileColumn = column & 8;

        }

        /** Unlike normal widgets,  */
        void renderRow(Coord row, Pixel * buffer, Coord startx, Coord numPixels) {
            UNIMPLEMENTED;            
        }

    private:
        // tilemap 
        static inline uint8_t tileMap_[40] = {
            '0', '0', ':', '0', '0', ' ', ' ', ' ', 
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
        };
        static inline uint8_t colorOffsets[40] = {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
        }
    }; // rckid::ui::Header

} // namespace rckid::ui