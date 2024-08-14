#include <platform/tests.h>
#include <rckid/graphics/bitmap.h>

using namespace rckid;

TEST(bitmap, pixels_bpp8) {
    memoryEnterArena();
    Bitmap<Color256> bmp{320, 240};
    Color256 x{78};
    Color256 y{56};
    bmp.setPixelAt(10, 67, x);
    bmp.setPixelAt(5, 57, y);
    EXPECT(bmp.pixelAt(10, 67) == x);
    EXPECT(bmp.pixelAt(5, 57) == y);

    memoryLeaveArena();
}

TEST(bitmap, pixels_bpp4) {
    memoryEnterArena();
    Bitmap<Color16> bmp{320, 240};
    Color16 x{13};
    Color16 y{8};
    bmp.setPixelAt(10, 67, x);
    bmp.setPixelAt(5, 57, y);
    EXPECT(bmp.pixelAt(10, 67) == x);
    EXPECT(bmp.pixelAt(5, 57) == y);

    bmp.setPixelAt(319, 0, x);
    bmp.setPixelAt(319, 1, y);
    EXPECT(bmp.pixelAt(319, 0) == x);
    EXPECT(bmp.pixelAt(319, 1) == y);
    EXPECT(bmp.rawBuffer()[0] == 0x8d);


    memoryLeaveArena();
}
