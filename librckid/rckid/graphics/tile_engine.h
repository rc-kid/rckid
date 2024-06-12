#pragma once

#include "rckid/rckid.h"
#include "rckid/graphics/color.h"
#include "rckid/graphics/primitives.h"

#include "tile.h"

namespace rckid {

    /** Simple tile engine with very low memory overhead.  
     
        Supports only one layer with no non-display area and a few sprites. Due to its low memory footprint, the engine is particularly useful for simple UI applications. 
     */
    template<typename TILE>
    class SimpleEngine {
    public:
        using Tile = TILE;

        struct TileInfo {
            char c = ' ';
            uint8_t paletteOffset = 0;

            TileInfo & operator = (char c) { this->c = c; return *this; }
        }; // SimpleEngine::TileInfo

        SimpleEngine(int width, int height):
            w_{width}, 
            h_{height},
            tileMap_{ new TileInfo[width * height] },
            buffer_{ new uint16_t[height * Tile::Height * 2]} {
            ASSERT(width > 0 && pixelWidth() <= 320);
            ASSERT(height > 0 && pixelHeight() <= 240);
            top_ = (240 - pixelHeight()) / 2;
        }

        ~SimpleEngine() {
            delete tileMap_;
            delete buffer_;
        }

        Tile const * tiles() const { return tiles_; }
        void setTiles(Tile const * tiles) { tiles_ = tiles; }

        ColorRGB const * palette() const { return palette_; }
        void setPalette(ColorRGB const * palette) { palette_ = palette; }

        int width() const { return w_; }
        int height() const { return h_; }

        int pixelWidth() const { return w_ * Tile::Width; }
        int pixelHeight() const { return h_ * Tile::Height; }

        TileInfo const & at(int x, int y) const { return tileMap_[y + x * h_]; }
        TileInfo & at(int x, int y) { return tileMap_[y + x * h_]; }

        /** Fills the entire tileset with given value. 
         */
        void fill(char c, uint8_t paletteOffset = 0) {
            for (int i = 0, e = w_ * h_; i != e; ++i)
                tileMap_[i] = TileInfo{c, paletteOffset};
        }

        /** Single-line writer interface. 
         */
        Writer text(int x, int y, uint8_t paletteOffset = 0) {
            return Writer{[this, x, y, paletteOffset](char c) mutable {
                at(x++, y) = TileInfo{c, paletteOffset};
            }};
        }

        /** \name Rendering 
         
            The rendering is done per column in alternating parts of the render buffer so that while the DMA is sending one column to the display, the next column is being calculated. 
         */
        //@{

        void enable() {
            ST7789::configure(DisplayMode::Native_RGB565);
            ST7789::enterContinuousUpdate(Rect::XYWH((320 - pixelWidth()) / 2, top_, pixelWidth(), pixelHeight()));
        }

        void disable() { 
            ST7789::leaveContinuousUpdate();
        }

        void render() {
            renderColumn_ = pixelWidth() - 1;
            renderColumn(renderColumn_, bufferForColumn(renderColumn_));
            --renderColumn_;
            renderColumn(renderColumn_, bufferForColumn(renderColumn_));
            ST7789::waitVSync();
            ST7789::writePixels(bufferForColumn(renderColumn_ + 1), pixelHeight(), [this]() {
                if (renderColumn_ < 0)
                    return true;
                // write already processed pixels
                ST7789::writePixels(bufferForColumn(renderColumn_), pixelHeight());
                if (--renderColumn_ >= 0)
                    renderColumn(renderColumn_, bufferForColumn(renderColumn_));
                return false;
            });
        }
        //@}

    protected:

        /** Returns the buffer part to be used for given column as the buffer is twice the column height. 
         */
        uint16_t * bufferForColumn(int col) {
            return buffer_ + (col & 1) * pixelHeight();
        }

        /** Renders the given column. 
         */
        void renderColumn(int x, uint16_t * buffer) {
            // get the tile x coordinate and the x coordinate within the tile
            int tx = x / Tile::Width;
            x = x % Tile::Width;
            // get first tile info for the given column (the column is consecutive tiles), and iterate over the tiles
            TileInfo const * tile = tileMap_ + (tx * h_);
            uint16_t * bx = buffer;
            for (int ty = 0; ty < h_; ++ty, ++tile)
                bx = tiles_[static_cast<uint8_t>(tile->c)].renderColumn(x, bx, palette_, tile->paletteOffset);
            // TODO render sprites & bitmaps, if any 
        }

        int w_;
        int h_;
        int top_;

        Tile const * tiles_ = nullptr;

        ColorRGB const * palette_ = nullptr; 

        TileInfo * tileMap_;
        // single column rendering buffer
        uint16_t * buffer_;
        // column to be rendered
        int renderColumn_;

    }; // rckid::SimpleEngine

} // namespace rckid

