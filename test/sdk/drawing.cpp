#include <platform/tests.h>
#include <rckid/graphics/drawing.h>

using namespace rckid;

TEST(drawing, pixelBufferSize) {
    EXPECT(pixelBufferSize<ColorRGB>(320, 240) == 320 * 240 * 2);
    EXPECT(pixelBufferSize<Color256>(320, 240) == 320 * 240);
    EXPECT(pixelBufferSize<Color16>(320, 240) == 320 * 240 / 2);
}

TEST(drawing, pixelOffset) {
    EXPECT(pixelOffset(319, 0, 320, 240) == 0);
    EXPECT(pixelOffset(319, 1, 320, 240) == 1);

    EXPECT(pixelOffset(0, 239, 320, 240) == 320 * 240 - 1);
}

TEST(drawing, setPixelAt) {
}