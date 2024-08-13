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
    EXPECT(bmp.pixelAt(5, 57) == x);

    memoryLeaveArena();
}
