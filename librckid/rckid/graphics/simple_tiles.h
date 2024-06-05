#pragma once

#include "ST7789.h"

#include "assets/tiles/Iosevka_Tiles_12x24.h"
namespace rckid {

    /** Very simple tile engine for UI elements with minimal memory footprint. 
     
        Utilizes a single tilemap with no hidden areas and tilemap with palette offsets for different color schemes. Mainly useful for UI overlays, where the sprites and tiles are all stored in ROM, but will work from any memory location. 
     */
    class SimpleTiles {
    public:

        static constexpr int Width = 12;
        static constexpr int Height = 24;

        /** Tile info consists of the tile ID itself (char) and palette offset */
        class TileInfo {
        public:
            char c = ' ';
            uint8_t offset = 0;

            TileInfo & operator = (char ch) { c = ch; return *this; }

        } __attribute__((packed)); 

        SimpleTiles(int w = 320 / Width, int h = 240 / Height):
            w_{w}, 
            h_{h}, 
            tileMap_{new TileInfo[w * h]},
            buffer_{new uint16_t[h * Height * 2]} {
        }

        ~SimpleTiles() {
            delete [] tileMap_;
        }

        uint16_t const * palette() const { return palette_; }
        void setPalette(uint16_t const * palette) { palette_ = palette; }

        void set(int x, int y, char c) {
            ASSERT(x >= 0 && x < w_);
            ASSERT(y >= 0 && y < h_);
            tileMap_[y + (w_ - x - 1) * h_] = c;
        }

        void setTile(int x, int y, char c, uint8_t paletteOffset) {
            ASSERT(x >= 0 && x < w_);
            ASSERT(y >= 0 && y < h_);
            tileMap_[y + (w_ - x - 1) * h_] = TileInfo{c, paletteOffset};
        }

        char get(int x, int y) const {
            ASSERT(x >= 0 && x < w_);
            ASSERT(y >= 0 && y < h_);
            return tileMap_[y + (w_ - x - 1) * h_].c;
        }

        TileInfo const & getTile(int x, int y) const {
            ASSERT(x >= 0 && x < w_);
            ASSERT(y >= 0 && y < h_);
            return tileMap_[y + (w_ - x - 1) * h_];
        }

        void render() {
            ST7789::waitVSync();
            for (int col = w_ * Width -1 ; col >= 0; --col) {
                renderColumn(col, buffer_);
                ST7789::writePixels(buffer_, h_ * Height);
            }
        }

    private:

        void renderColumn(int x, uint16_t * buffer) {
            // determine the corresponding tilemap coordinate
            int tx = x / Width;
            // convert the x coordinate to the x coordinate inside the tile 
            x = x % Width;
            for (int y = 0; y < h_; ++y) {
                TileInfo const & tinfo = tileMap_[y + (w_ - tx - 1) * h_];
                unsigned offset = static_cast<uint8_t>(tinfo.c) * (Width * Height / 8);
                uint32_t const * tile = tileset_ + offset;
                tile = tile +  3 * x;
                uint32_t x = tile[0];
                for (int i = 0; i < 8; ++i) {
                    *(buffer++) = palette_[(x & 0xf) + tinfo.offset];
                    x = x >> 4;
                }
                x = tile[1];
                for (int i = 0; i < 8; ++i) {
                    *(buffer++) = palette_[(x & 0xf) + tinfo.offset];
                    x = x >> 4;
                }
                x = tile[2];
                for (int i = 0; i < 8; ++i) {
                    *(buffer++) = palette_[(x & 0xf) + tinfo.offset];
                    x = x >> 4;
                }
            }
        }


        // width and height in tiles
        int w_;
        int h_;

        TileInfo * tileMap_ = nullptr;

        uint32_t const * tileset_ = Iosevka_Tiles_12x24::tileset; 

        uint16_t const * palette_ = Palette_332_to_565;

        uint16_t * buffer_ = nullptr;
        


    }; // rckid::SimpleTiles 


} // namespace rckid