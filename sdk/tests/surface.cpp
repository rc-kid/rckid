#include <platform/tests.h>
#include <rckid/graphics/pixel_surface.h>
#include <rckid/graphics/color.h>

using namespace rckid;

TEST(surface, Size) {
    EXPECT(PixelSurface<1>::numBytes(320, 240) == 320 * 240 / 8);
    EXPECT(PixelSurface<2>::numBytes(320, 240) == 320 * 240 / 4);
    EXPECT(PixelSurface<4>::numBytes(320, 240) == 320 * 240 / 2);
    EXPECT(PixelSurface<8>::numBytes(320, 240) == 320 * 240);
    EXPECT(PixelSurface<16>::numBytes(320, 240) == 320 * 240 * 2);
}

TEST(surface, pixelAt) {
    uint16_t pixels[128];
    uint8_t * p = reinterpret_cast<uint8_t *>(pixels);
    for (uint32_t i = 0; i < 16 * 16; ++i)
        p[i] = i;
    EXPECT(PixelSurface<16>::pixelAt(15, 0, 16, 8, pixels) == 0x0100);
    EXPECT(PixelSurface<16>::pixelAt(15, 1, 16, 8, pixels) == 0x0302);
}
