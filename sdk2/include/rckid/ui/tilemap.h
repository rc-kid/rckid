#pragma once

#include <rckid/graphics/tile.h>
#include <rckid/ui/widget.h>

namespace rckid::ui {


    class TileInfo {
    public:

        TileInfo() = default;
        TileInfo(TileInfo const &) = default;
        TileInfo(char c): tile_{c} {}

        TileInfo & operator = (char c) { tile_ = c; return *this; }

        uint8_t tile() const { return tile_; }
        uint8_t paletteOffset() const { return paletteOffset_; }
        bool transparent() const { return (flags_ & TRANSPARENT_MASK) != 0; }
        uint8_t tileset() const { return flags_ & ALT_TILESET_MASK; }

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
            tile_ = ' ';
            paletteOffset_ = 0;
            flags_ = TRANSPARENT_MASK;
            return *this;
        }
    private:
        static constexpr uint8_t TRANSPARENT_MASK = 0x04;
        static constexpr uint8_t ALT_TILESET_MASK = 0x03;

        uint8_t tile_ = ' ';
        uint8_t paletteOffset_ = 0;
        uint8_t flags_ = TRANSPARENT_MASK;
        uint8_t reserved3;
    } __attribute__((packed)); // rckid::ui::TileInfo

    static_assert(sizeof(TileInfo) == 4);

    template<typename TILE>
    class Tilemap {
    public:

        Tilemap(Coord cols, Coord rows, TILE const * tileSet):
            cols_{cols}, 
            rows_{rows}, 
            tileMap_{new TileInfo[cols * rows]},
            tileSet_{tileSet, tileSet, tileSet, tileSet}, 
            palette_{nullptr} 
        {
        }

        Coord cols() const { return cols_; }
        Coord rows() const { return rows_; }

        Coord width() const { return cols_ * TILE::width(); }
        Coord height() const { return rows_ * TILE::height(); }

        TileInfo const & at(Coord x, Coord y) const {
            ASSERT(x < cols_ && y < rows_);
            return tileMap_.get()[mapIndexColumnFirst(x, y, cols_, rows_)];
        }

        TileInfo & at(Coord x, Coord y) {
            ASSERT(x < cols_ && y < rows_);
            return tileMap_.get()[mapIndexColumnFirst(x, y, cols_, rows_)];
        }


        Writer text(Coord x, Coord y) {
            ASSERT(x < cols_ && y < rows_);
            return Writer{[this, x, y](char c, void * arg) mutable {
                at(x++, y) = c;
            }, this};
        }

        void renderColumn(Coord column, Coord startRow, Coord numPixels, Color::RGB565 * buffer) {
            Coord col = column / TILE::width();
            if (col < 0 || col >= cols_)
                return;
            Coord tileCol = column % TILE::width();
            TileInfo const * ti = tileMap_.get() + mapIndexColumnFirst(col, 0, cols_, rows_);
            // skip any tiles we do not want to render
            if (startRow >= TILE::height()) {
                ti += startRow / TILE::height() * cols_;
                startRow %= TILE::height();
            }
            while (numPixels > 0) {
                TILE const * tileset = tileSet_[ti->tileset()];
                Coord drawPixels = std::min(numPixels, TILE::height() - startRow);
                if (ti->transparent())
                    tileset->renderColumnTransparent(tileCol, startRow, drawPixels, buffer, 0, palette_.ptr() + ti->paletteOffset());
                else
                    tileset->renderColumn(tileCol, startRow, drawPixels, buffer, 0, palette_.ptr() + ti->paletteOffset());
                numPixels -= drawPixels;
                startRow = 0;
                buffer += drawPixels;
                ++ti;
            }
        }

    private:
        Coord cols_;
        Coord rows_;
        unique_ptr<TileInfo> tileMap_;
        mutable_ptr<Color::RGB565> palette_;
        TILE const * tileSet_[4];

    }; // rckid::ui::Tilemap<TILE>

} // namespace rckid::ui