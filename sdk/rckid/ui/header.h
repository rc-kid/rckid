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
        Header() : Widget{Rect::XYWH(0, 0, 320, 16)} {}

    protected:
        /** Renders the header by column. 
         
            
         */
        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            uint32_t tile = column / 8;
            uint32_t tileColumn = column % 8;
            assets::Iosevka16[tileMap_[tile]].renderColumn(tileColumn, starty, numPixels, buffer, 0, palette_);
        }

        /** Unlike normal widgets,  */
        void renderRow(Coord row, uint16_t * buffer, Coord startx, Coord numPixels) {
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
        };

        static constexpr uint16_t palette_[] = {
            0x0000, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
            0xffff, 
        };
    }; // rckid::ui::Header

} // namespace rckid::ui