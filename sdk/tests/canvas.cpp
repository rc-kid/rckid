#include <platform/tests.h>
#include <rckid/graphics/canvas.h>

using namespace rckid;

TEST(canvas, color256) {
    Canvas<Color256> bmp{320, 240};
    Canvas<Color256>::Pixel x{78};
    Canvas<Color256>::Pixel y{56};
    bmp.setAt(10, 67, x);
    bmp.setAt(5, 57, y);
    EXPECT(bmp.at(10, 67) == x);
    EXPECT(bmp.at(5, 57) == y);
}

TEST(canvas, colorRGB) {
    Canvas<ColorRGB> bmp{320, 240};
    Canvas<ColorRGB>::Pixel x = ColorRGB::fromRaw(13);
    Canvas<ColorRGB>::Pixel y = ColorRGB::fromRaw(8);
    bmp.setAt(10, 67, x);
    bmp.setAt(5, 57, y);
    EXPECT(bmp.at(10, 67) == x);
    EXPECT(bmp.at(5, 57) == y);

    bmp.setAt(319, 0, x);
    bmp.setAt(319, 1, y);
    EXPECT(bmp.at(319, 0) == x);
    EXPECT(bmp.at(319, 1) == y);
    EXPECT(bmp.pixels()[0] == x.toRaw());
    EXPECT(bmp.pixels()[1] == y.toRaw());
}

/*
TEST(bitmap, arena_allocator_8bpp) {
    using namespace rckid;
    ArenaGuard g_;
    EXPECT(Arena::currentSize() == 0);
    Bitmap<Color256> bmp{320, 240, Arena::allocator()};
    EXPECT(Arena::currentSize() == 320 * 240); 
    Bitmap<Color256>::Pixel x{78};
    Bitmap<Color256>::Pixel y{56};
    bmp.setAt(10, 67, x);
    bmp.setAt(5, 57, y);
    EXPECT(bmp.at(10, 67) == x);
    EXPECT(bmp.at(5, 57) == y);
}
    */


