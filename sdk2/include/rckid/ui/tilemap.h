#pragma once

#include <rckid/graphics/tile.h>
#include <rckid/ui/widget.h>

namespace rckid::ui {


    class TileInfo {
    public:
        uint8_t tileIndex;
        uint8_t paletteOffset;
    private:
        uint8_t reserved2;
        uint8_t reserved3;
    } __attribute__((packed)); // rckid::ui::TileInfo

    static_assert(sizeof(TileInfo) == 4);

    template<typename TILE>
    class Tilemap : public Widget {
    public:
        


    private:
        unique_ptr<TileInfo> tileMap_;

    }; // rckid::ui::Tilemap<TILE>

} // namespace rckid::ui