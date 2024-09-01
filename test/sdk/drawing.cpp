#include <platform/tests.h>
#include <rckid/graphics/drawing.h>

using namespace rckid;

TEST(drawing, pixelBufferSize) {
    EXPECT(pixelBufferSize<ColorRGB>(320, 240) == 320 * 240 * 2);
    EXPECT(pixelBufferSize<Color256>(320, 240) == 320 * 240);
    EXPECT(pixelBufferSize<Color16>(320, 240) == 320 * 240 / 2);
}

TEST(drawing, pixelBufferOffset) {
    EXPECT(pixelBufferOffset(319, 0, 320, 240) == 0);
    EXPECT(pixelBufferOffset(319, 1, 320, 240) == 1);

    EXPECT(pixelBufferOffset(0, 239, 320, 240) == 320 * 240 - 1);
}

TEST(drawing, setPixelBufferAt) {
}




TEST(drawing, pixelBufferColumnOffset_256) {
    EXPECT(pixelBufferColumnOffset<Color256>(8,8,7) == 0);
    EXPECT(pixelBufferColumnOffset<Color256>(8,8,6) == 8);
    EXPECT(pixelBufferColumnOffset<Color256>(8,8,5) == 16);
    EXPECT(pixelBufferColumnOffset<Color256>(8,8,4) == 24);
    EXPECT(pixelBufferColumnOffset<Color256>(8,8,3) == 32);
    EXPECT(pixelBufferColumnOffset<Color256>(8,8,2) == 40);
    EXPECT(pixelBufferColumnOffset<Color256>(8,8,1) == 48);
    EXPECT(pixelBufferColumnOffset<Color256>(8,8,0) == 56);
}

TEST(drawing, pixelBufferColumnOffset_16) {
    EXPECT(pixelBufferColumnOffset<Color16>(4,8,3) == 0);
    EXPECT(pixelBufferColumnOffset<Color16>(4,8,2) == 4);
    EXPECT(pixelBufferColumnOffset<Color16>(4,8,1) == 8);
    EXPECT(pixelBufferColumnOffset<Color16>(4,8,0) == 12);
}
