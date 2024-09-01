#include <platform/tests.h>
#include <rckid/graphics/tile.h>
#include <rckid/assets/tiles/ui-tiles.h>

using namespace rckid;

TEST(tile, tile_8_8_256) {
    Tile<8,8,Color256> t({
      1, 0, 0, 0, 0, 0, 0, 0,
      0, 2, 0, 0, 0, 0, 0, 0,
      0, 0, 3, 0, 0, 0, 0, 0,
      0, 0, 0, 4, 0, 0, 0, 0,
      0, 0, 0, 0, 5, 0, 0, 0,
      0, 0, 0, 0, 0, 6, 0, 0,
      0, 0, 0, 0, 0, 0, 7, 0,
      0, 0, 0, 0, 0, 0, 0, 8,
    });

    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            EXPECT(t.pixelAt(i,j) == ((i == j) ? i + 1 : 0));

    t.fill(Color256{255});
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            EXPECT(t.pixelAt(i,j) == 255);
}

TEST(tile, tile_8_8_16) {
    Tile<8,8,Color16> t({
      1, 0, 0, 0, 0, 0, 0, 0,
      0, 2, 0, 0, 0, 0, 0, 0,
      0, 0, 3, 0, 0, 0, 0, 0,
      0, 0, 0, 4, 0, 0, 0, 0,
      0, 0, 0, 0, 5, 0, 0, 0,
      0, 0, 0, 0, 0, 6, 0, 0,
      0, 0, 0, 0, 0, 0, 7, 0,
      0, 0, 0, 0, 0, 0, 0, 8,
    });

    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            EXPECT(t.pixelAt(i,j) == ((i == j) ? i + 1 : 0));

    t.fill(Color16{15});
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            EXPECT(t.pixelAt(i,j) == 15);
}

TEST(tile, tile_12_24_16) {
    Tile<12,24,Color16> t({
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    });

    for (int i = 0; i < 12; ++i)
        for (int j = 0; j < 24; ++j)
            EXPECT(t.pixelAt(i,j) == ((i == j) ? i + 1 : 0));

    t.fill(Color16{15});
    for (int i = 0; i < 12; ++i)
        for (int j = 0; j < 24; ++j)
            EXPECT(t.pixelAt(i,j) == 15);
}


TEST(tile, tile_8_8_16_render) {
    Tile<8,8,Color16> t({
      1, 0, 0, 0, 0, 0, 0, 0,
      0, 2, 0, 0, 0, 0, 0, 1,
      0, 0, 3, 0, 0, 0, 0, 2,
      0, 0, 0, 4, 0, 0, 0, 3,
      0, 0, 0, 0, 5, 0, 0, 4,
      0, 0, 0, 0, 0, 6, 0, 5,
      0, 0, 0, 0, 0, 0, 7, 6,
      0, 0, 0, 0, 0, 0, 0, 7,
    });

    ColorRGB out[8];
    ColorRGB palette[] = {
        color::Black,
        color::Blue,
        color::Green,
        color::Red,
        color::Yellow,
        color::Violet,
        color::Cyan,
        color::DarkGray,
    };
    ColorRGB * outX = t.renderColumn(7, out, palette, 0);
    EXPECT(outX == out + 8);
    for (int i = 0; i < 8; ++i)
        EXPECT(out[i] == palette[i]);
}


