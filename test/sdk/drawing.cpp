#include <platform/tests.h>
#include <rckid/graphics/drawing.h>

using namespace rckid;

TEST(drawing, pixelOffset) {
    EXPECT(pixelOffset(319, 0, 320, 240) == 0);
    EXPECT(pixelOffset(319, 1, 320, 240) == 1);
}