#pragma once

#include <rckid/graphics/tile.h>

#include <assets/Iosevka16Tiles.h>
#include <assets/System16Tiles.h>

namespace rckid {

    /** Tile grid is a more primitive counterpart to a tilemap.
     
        It provides a single tile layer for a tiles of fixed size (8x16 pixels), which is just enough for a readable fornt on RCKid. Its purpose is to provide cheap and efficient rendering of larger-ish text-like features, such as multi-line labels, headers, etc. 
     */
    class TileGrid {
    public:
        using Tile = rckid::Tile<8,16,Color::Index16>;

        class TileInfo {
        public:

            TileInfo() = default;
            TileInfo(TileInfo const &) = default;
            TileInfo(char c): tile_{c} {}

            /** Sets tile info to given character. 
             
                As tilesets start from 0, while printable characters technically start from 32, we substract 32 from the character to ensure first printable character is also the first tile in the tileset, which minimizes the amount of memory (flash or ram), needed for the tileset. 
             */
            TileInfo & operator = (char c) { tile_ = c - 32; return *this; }

            TileInfo & operator = (uint32_t tileIndex) { 
                ASSERT(tileIndex <= 255);
                tile_ = static_cast<uint8_t>(tileIndex); 
                return *this; 
            }

            uint8_t tile() const { return tile_; }
            uint8_t paletteOffset() const { return paletteOffset_; }
            bool transparent() const { return (flags_ & TRANSPARENT_MASK) != 0; }
            bool  altTileset() const { return flags_ & ALT_TILESET_MASK; }

            TileInfo & setPaletteOffset(uint8_t paletteOffset) { 
                paletteOffset_ = paletteOffset; 
                return *this;
            }

            TileInfo & setTransparent(bool transparent = true) {
                if (transparent)
                    flags_ |= TRANSPARENT_MASK;
                else
                    flags_ &= ~TRANSPARENT_MASK;
                return *this;
            }

            TileInfo & setTileset(uint8_t altTileset) {
                ASSERT(altTileset <= ALT_TILESET_MASK);
                flags_ = (flags_ & ~ALT_TILESET_MASK) | altTileset;
                return *this;
            }

            TileInfo & clear() {
                tile_ = 0;
                paletteOffset_ = 0;
                flags_ = TRANSPARENT_MASK;
                return *this;
            }
        private:
            static constexpr uint8_t TRANSPARENT_MASK = 0x02;
            static constexpr uint8_t ALT_TILESET_MASK = 0x01;

            uint8_t tile_ = 0;
            uint8_t paletteOffset_ = 0;
            uint8_t flags_ = TRANSPARENT_MASK;
            uint8_t reserved3;
        } __attribute__((packed)); // rckid::TileGrid::TileInfo

        static_assert(sizeof(TileInfo) == 4);


        TileGrid(Coord cols, Coord rows, mutable_ptr<Color::RGB565> palette):
            cols_{cols}, 
            rows_{rows}, 
            grid_{new TileInfo[cols * rows]},
            palette_{std::move(palette)} 
        {
        }

        Coord cols() const { return cols_; }
        Coord rows() const { return rows_; }

        Coord width() const { return cols_ * Tile::width(); }
        Coord height() const { return rows_ * Tile::height(); }

        TileInfo const & at(Coord x, Coord y) const {
            ASSERT(x < cols_ && y < rows_);
            return grid_.get()[mapIndexColumnFirst(x, y, cols_, rows_)];
        }

        TileInfo & at(Coord x, Coord y) {
            ASSERT(x < cols_ && y < rows_);
            return grid_.get()[mapIndexColumnFirst(x, y, cols_, rows_)];
        }


        /** Returns writer for outputting single-line text to the tilemap. 
         
            The text *must* fill into the tile grid's dimensions, otherwise the behavior is undefined (there will be memory corruption). 
         */
        Writer text(Coord x, Coord y) {
            ASSERT(x < cols_ && y < rows_);
            return Writer{[this, x, y](char c) mutable {
                at(x++, y) = c;
            }};
        }

        void renderColumn(Coord column, Coord startRow, Coord numPixels, Color::RGB565 * buffer) {
            Coord col = column / Tile::width();
            if (col < 0 || col >= cols_)
                return;
            Coord tileCol = column % Tile::width();
            TileInfo const * ti = grid_.get() + mapIndexColumnFirst(col, 0, cols_, rows_);
            // skip any tiles we do not want to render
            if (startRow >= Tile::height()) {
                ti += startRow / Tile::height() * cols_;
                startRow %= Tile::height();
            }
            while (numPixels > 0) {
                Tile const * tileset = ti->altTileset() ? assets::System16Tiles : assets::Iosevka16Tiles;
                Coord drawPixels = std::min(numPixels, Tile::height() - startRow);
                if (ti->transparent())
                    tileset[ti->tile()].renderColumn(tileCol, startRow, drawPixels, buffer, palette_.ptr() + ti->paletteOffset(), 0);
                else
                    tileset[ti->tile()].renderColumn(tileCol, startRow, drawPixels, buffer, palette_.ptr() + ti->paletteOffset());
                numPixels -= drawPixels;
                startRow = 0;
                buffer += drawPixels;
                ++ti;
            }
        }

    private:
        Coord cols_;
        Coord rows_;
        unique_ptr<TileInfo> grid_;
        mutable_ptr<Color::RGB565> palette_;

    }; // rckid::TileGrid

} // namespace rckid