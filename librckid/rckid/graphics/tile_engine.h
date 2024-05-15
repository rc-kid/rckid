#pragma once

#include "rckid/rckid.h"
#include "rckid/graphics/color.h"
#include "rckid/graphics/primitives.h"

namespace rckid {

    /** Single Tile

        Tiles are 16x16 pixels and use 256 colors from a custom palette with the color 0 being transparent 
     */
    class Tile {
    public:

        uint8_t at(int x, int y) const { return data_[y + x * 16]; }
        uint8_t & at(int x, int y) { return data_[y + x * 16]; }

    private:

        uint8_t data_[16 * 16];

    }; // rckid::Tile

    class TileMap {
    public:

        TileMap(int width, int height, Tile * tileSet = nullptr, ColorRGB * palette = nullptr):
            width_{width}, 
            height_{height},
            tileMap_{ new uint8_t[width * height]} {

        }

        int width() const { return width_; }
        int height() const { return height_; }
        int top() const { return top_; }
        int left() const { return left_; }
        Point topLeft() const { return Point{left_, top_}; }

        uint8_t at(int x, int y) const { return tileMap_[y + x * height_]; }
        uint8_t & at(int x, int y) { return tileMap_[y + x * height_]; }

        Tile const * tileSet() const { return tileSet_; }
        Tile * tileSet() { return tileSet_; }

        void setTileSet(Tile * value) { tileSet_ = value;}

        ColorRGB const * palette() const { return palette_; }
        ColorRGB * palette() { return palette_; }

        void setPalette(ColorRGB * value) { palette_ = value; }


    private:
        friend class TileEngine;

        int width_;
        int height_;
        int top_ = 0;
        int left_ = 0;
        uint8_t * tileMap_;

        Tile * tileSet_;
        ColorRGB * palette_;

    }; 

    /** Tile Engine with three layers and some sprites. 
     
        needs to:
        1) draw the tiles
        2) convert to RGB
        2) draw the sprites
     */
    class TileEngine {
    public:

        // TODO default to some basic palette and to a tileset that makes some basic sense
        TileEngine():
            buffer_{new ColorRGB[240 + 32]},
            layers_{TileMap{30, 25}, TileMap{30,25}, TileMap{30,25}} {
        }

        void enable() {

        }

        void disable() {

        }

        void render() {

        }

        TileMap const & operator [] (unsigned index) const { return layers_[index]; }
        TileMap & operator [] (unsigned index) { return layers_[index]; }

    private:

        /** Renders the given column into provided buffer. 
         
            First render the background, uint32 by uint32 reads for fastest speed. This means we start this layer so that we start always at properly aligned boundary. 

            Then render the second layer by reading the tiles 4 by four, 

            TODO
         
        */
        void renderColumn(int col) {
            // first get the buffer that we use for the calculations. We use the same bufer as for the final calculation, but at only 1 byte per pixel, we start later in it.
            // the end of the buffer is where we store temporary palette results
            uint8_t * buffer8_ = reinterpret_cast<uint8_t*>(buffer_ + 136);
            // draw the first layer
            // be dumb and just copy the appropriate tile & pixels
            int x = layers_[0].left_ + col;
            int tileX = x / 16; // index of the tile
            x = x % 16; // index inside the tile
            int y = layers_[0].top_;
            int tileY = y / 16;
        }

        /** Renders the background layer, which is the simplest. Returns the offset in the buffer */
        int renderBackgroundLayer(TileMap & layer, uint32_t * buffer, int col) {
            int tile = layer.top_ / 16;
            int startRow = layer.top_ % 16;
            int column = (layer.left_ + col) % layer.width_;
            

            
        }

        ColorRGB * buffer_ = nullptr;
        TileMap layers_[3];
 
    };
}