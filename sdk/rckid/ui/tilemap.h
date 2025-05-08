#pragma once

#include "../graphics/surface.h"

namespace rckid::ui {

    /** Simple widget that draws a tilemap. 
     */
    template<typename TILE>
    class Tilemap : public Widget {
    public:
        PACKED(struct TileInfo {
            char c = ' ';
            uint8_t paletteOffset = 0;

            TileInfo() = default;

            TileInfo(char c, uint8_t paletteOffset = 0):
                c{c}, paletteOffset{paletteOffset} {
            }

            TileInfo & operator = (char c) { this->c = c; return *this; }
            TileInfo & setPaletteOffset(uint8_t paletteOffset) { this->paletteOffset = paletteOffset; return *this; }
        }); // Tilemap::TileInfo

        using Tile = TILE;

        Tilemap(Coord cols, Coord rows, Tile const * tileset, uint16_t const * palette):
            Widget{Rect::WH(cols * TILE::width(), rows * TILE::height())},
            cols_{cols}, 
            rows_{rows}, 
            tileSet_{tileset}, 
            palette_{palette} {
            tileMap_ = new TileInfo[cols * rows]; 
        }

        ~Tilemap() override {
            delete [] tileMap_;
        }

        Coord cols() const { return cols_; }
        Coord rows() const { return rows_; }
        Coord width() const { return cols_ * TILE::width(); }
        Coord height() const { return rows_ * TILE::height(); }

        TileInfo const & at(Coord x, Coord y) const {
            ASSERT(x < cols_ && y < rows_);
            return tileMap_[mapIndexColumnFirst(x, y, cols_, rows_)];
        }

        TileInfo & at(Coord x, Coord y) {
            ASSERT(x < cols_ && y < rows_);
            return tileMap_[mapIndexColumnFirst(x, y, cols_, rows_)];
        }

        /** Renders the given column, 
         */
        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            uint32_t tile = column / TILE::width();
            uint32_t tileColumn = column % TILE::width();
            TileInfo const * ti = tileMap_ + mapIndexColumnFirst(tile, 0, cols_, rows_);
            // skip any tiles we do not want to render
            if (starty >= TILE::height()) {
                ti += starty / TILE::height();
                starty %= TILE::height();
            }
            while (numPixels > 0) {
                uint32_t x = tileSet_[static_cast<uint8_t>(ti->c)].renderColumn(tileColumn, starty, numPixels > TILE::height() ? TILE::height() : numPixels, buffer, 0, palette_ + ti->paletteOffset);
                numPixels -= x;
                starty = 0;
                buffer += x;
                ++ti;
            }
        }

        void fill(char c, uint8_t paletteOffset = 0) {
            for (uint32_t i = 0, e = cols_ * rows_; i != e;  ++i)
                tileMap_[i] = TileInfo{c, paletteOffset};
        }

        Writer text(Coord x, Coord y) {
            ASSERT(x < cols_ && y < rows_);
            return Writer{[x, y, this](char c) mutable {
                at(x++, y) = c;
            }};
        }

    private:

        static_assert(sizeof(TileInfo) == 2);

        Coord cols_;
        Coord rows_;
        TileInfo * tileMap_ = nullptr;
        Tile const * tileSet_ = nullptr;
        uint16_t const * palette_; 

    }; // rckid::ui::Tilemap<TILE>

} // namespace rckid::ui