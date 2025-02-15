#include <platform/tests.h>
#include <rckid/graphics/pixel_array.h>

using namespace rckid;

TEST(pixelArray, Size) {
    EXPECT(PixelArray<2>::numBytes(320, 240) == 320 * 240 / 4);
    EXPECT(PixelArray<4>::numBytes(320, 240) == 320 * 240 / 2);
    EXPECT(PixelArray<8>::numBytes(320, 240) == 320 * 240);
    EXPECT(PixelArray<16>::numBytes(320, 240) == 320 * 240 * 2);
}

TEST(pixelArray, offset) {
    EXPECT(PixelArray<8>::offset(319, 0, 320, 240) == 0);
    EXPECT(PixelArray<8>::offset(319, 1, 320, 240) == 1);
    EXPECT(PixelArray<8>::offset(0, 239, 320, 240) == 320 * 240 - 1);

    EXPECT(PixelArray<16>::offset(319, 0, 320, 240) == 0);
    EXPECT(PixelArray<16>::offset(319, 1, 320, 240) == 1);
    EXPECT(PixelArray<16>::offset(0, 239, 320, 240) == 320 * 240 - 1);
}
