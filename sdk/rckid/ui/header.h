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
            assets::system16[tileMap_[tile]].renderColumn(tileColumn, starty, numPixels, buffer, 0, palette_ + 16 * colorOffsets_[tile]);
        }

        /** Unlike normal widgets,  */
        void renderRow(Coord row, uint16_t * buffer, Coord startx, Coord numPixels) {
            UNIMPLEMENTED;            
        }

    private:
        // tilemap 
        static inline uint8_t tileMap_[40] = {
            '0', '0', ':', '0', '0', ' ', '0', '1', 
            '2', '3', '4', '5', '6', '7', '8', '9', 
            ' ', ' ', ' ', ' ', 'R', 'C', 'K', 'i', 
            'd', ' ', 'm', 'k', 'I', 'I', 'I', ' ', 
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 
        };
        static inline uint8_t colorOffsets_[40] = {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
        };

        static constexpr uint16_t palette_[] = {
            // gray
            ColorRGB{0x00, 0x00, 0x00}.toRaw(), 
            ColorRGB{0x11, 0x11, 0x11}.toRaw(), 
            ColorRGB{0x22, 0x22, 0x22}.toRaw(), 
            ColorRGB{0x33, 0x33, 0x33}.toRaw(), 
            ColorRGB{0x44, 0x44, 0x44}.toRaw(), 
            ColorRGB{0x55, 0x55, 0x55}.toRaw(), 
            ColorRGB{0x66, 0x66, 0x66}.toRaw(), 
            ColorRGB{0x77, 0x77, 0x77}.toRaw(), 
            ColorRGB{0x88, 0x88, 0x88}.toRaw(), 
            ColorRGB{0x99, 0x99, 0x99}.toRaw(), 
            ColorRGB{0xaa, 0xaa, 0xaa}.toRaw(), 
            ColorRGB{0xbb, 0xbb, 0xbb}.toRaw(), 
            ColorRGB{0xcc, 0xcc, 0xcc}.toRaw(), 
            ColorRGB{0xdd, 0xdd, 0xdd}.toRaw(), 
            ColorRGB{0xee, 0xee, 0xee}.toRaw(), 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            0x2222, 
            0x3333, 
            0x4444, 
            0x5555, 
            0x6666, 
            0x7777, 
            0x8888, 
            0x9999, 
            0xaaaa, 
            0xbbbb, 
            0xcccc, 
            0xdddd, 
            0xeeee, 
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