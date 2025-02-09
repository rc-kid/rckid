#include <platform/tests.h>
#include <rckid/graphics/bitmap.h>

using namespace rckid;

TEST(bitmap, pixels_bpp8) {
    Bitmap<8> bmp{320, 240};
    Bitmap<8>::Pixel x{78};
    Bitmap<8>::Pixel y{56};
    bmp.setAt(10, 67, x);
    bmp.setAt(5, 57, y);
    EXPECT(bmp.at(10, 67) == x);
    EXPECT(bmp.at(5, 57) == y);
}

TEST(bitmap, pixels_bpp4) {
    Bitmap<16> bmp{320, 240};
    Bitmap<16>::Pixel x{13};
    Bitmap<16>::Pixel y{8};
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
    EXPECT(Arena::currentSize() == 0);
    Bitmap<8> bmp{320, 240, Arena::allocator()};
    EXPECT(Arena::currentSize() == 320 * 240); 
    Bitmap<8>::Pixel x{78};
    Bitmap<8>::Pixel y{56};
    bmp.setAt(10, 67, x);
    bmp.setAt(5, 57, y);
    EXPECT(bmp.at(10, 67) == x);
    EXPECT(bmp.at(5, 57) == y);
}

/*
TEST(bitmap, fromPng) {
    using namespace rckid;
    ArenaGuard g_;
    Bitmap<ColorRGB565> bmp{PNG::fromBuffer(nullptr, 0), Arena::allocator()};
}
*/

