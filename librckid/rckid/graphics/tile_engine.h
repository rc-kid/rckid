#pragma once 

#include "rckid/rckid.h"
#include "rckid/ST7789.h"
#include "rckid/graphics/color.h"
namespace rckid {

    /** Single tile. 
     */
    template<int W, int H, typename COLOR = ColorRGB> 
    class Tile {
        static constexpr int Height = H;
        static constexpr int Width = W;

    }; 

    class TileMap {
    public:

        /** Width and height of the tilemap. 
         */
        int width() const { return w_; }
        int height() const { return h_; }

        Point offset() const { return Point{x_, y_}; }

    private:
        int w_;
        int h_;
        int x_;
        int y_;
    }; 

    /** 2D tile engine GPU. 

        A column by column render stuff that displays the   
     
     */
    template<typename TILE, DisplayMode DISPLAY_MODE = DisplayMode::Native_RGB565>
    class TileEngine;

    template<typename TILE>
    class TileEngine<TILE, DisplayMode::Native_RGB565> {
    public:

        using Tile = TILE;

        void enable() {
            // allocate VRAM for the buffers
            renderBuffer1_ = reinterpret_cast<uint32_t*>((allocateVRAM(height() + Tile::Height) / 2));
            renderBuffer2_ = reinterpret_cast<uint32_t*>((allocateVRAM(height() + Tile::Height) / 2));
            ST7789::configure(DisplayMode::Native_RGB565);
            ST7789::enterContinuousUpdate();
        }

        void disable() {
            ST7789::leaveContinuousUpdate();
            // TODO deallocate VRAM
        }

        /** Renders the screen. 
         */
        void render() {
            column_ = 319;
            if (layer0_ != nullptr)
                column0_ = 

            ST7789::waitVSync();
        }

    protected:

        /** Renders the column. 
         */
        void renderColumn(uint32_t * buffer) {

        } 

        static constexpr int WIDTH = 320;
        static constexpr int HEIGHT = 240;

        TileMap<TILE> * layer0_ = nullptr;
        TileMap<TILE> * layer1_ = nullptr;
        TileMap<TILE> * layer2_ = nullptr;
        TileMap<TILE> * layer3_ = nullptr;

        uint32_t * renderBuffer1_ = nullptr;
        uint32_t * renderBuffer2_ = nullptr;
        int column_ = 319;
        Point pos0_;
        Point pos1_;
        Point pos2_;
        Point pos3_;


    }; // TileEngine

} // namespace rckid