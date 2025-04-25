#include <platform/tests.h>
#include <rckid/graphics/surface.h>

using namespace rckid;

TEST(surface, Size) {
    EXPECT(Surface<1>::numBytes(320, 240) == 320 * 240 / 8);
    EXPECT(Surface<2>::numBytes(320, 240) == 320 * 240 / 4);
    EXPECT(Surface<4>::numBytes(320, 240) == 320 * 240 / 2);
    EXPECT(Surface<8>::numBytes(320, 240) == 320 * 240);
    EXPECT(Surface<16>::numBytes(320, 240) == 320 * 240 * 2);
}

TEST(surface, pixelAt) {
    uint16_t pixels[128];
    uint8_t * p = reinterpret_cast<uint8_t *>(pixels);
    for (uint32_t i = 0; i < 16 * 16; ++i)
        p[i] = i;
    EXPECT(Surface<16>::pixelAt(pixels, 15, 0, 16, 8) == 0x0100);
    EXPECT(Surface<16>::pixelAt(pixels, 15, 1, 16, 8) == 0x0302);
}