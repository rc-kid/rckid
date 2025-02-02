#include <platform/tests.h>
#include <rckid/graphics/bitmap.h>

using namespace rckid;

TEST(bitmap, pixels_bpp8) {
    Bitmap<Color256> bmp{320, 240};
    Color256 x{78};
    Color256 y{56};
    bmp.setAt(10, 67, x);
    bmp.setAt(5, 57, y);
    EXPECT(bmp.at(10, 67) == x);
    EXPECT(bmp.at(5, 57) == y);
}

TEST(bitmap, pixels_bpp4) {
    Bitmap<Color16> bmp{320, 240};
    Color16 x{13};
    Color16 y{8};
    bmp.setAt(10, 67, x);
    bmp.setAt(5, 57, y);
    EXPECT(bmp.at(10, 67) == x);
    EXPECT(bmp.at(5, 57) == y);

    bmp.setAt(319, 0, x);
    bmp.setAt(319, 1, y);
    EXPECT(bmp.at(319, 0) == x);
    EXPECT(bmp.at(319, 1) == y);
    EXPECT(bmp.pixels()[0] == 0x8d);
}

TEST(bitmap, arena_allocator) {
    using namespace rckid;
    ArenaGuard g_;
    Bitmap<Color256, Arena> bmp{320, 240};
    Color256 x{78};
    Color256 y{56};
    bmp.setAt(10, 67, x);
    bmp.setAt(5, 57, y);
    EXPECT(bmp.at(10, 67) == x);
    EXPECT(bmp.at(5, 57) == y);
}

