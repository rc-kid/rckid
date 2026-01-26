#include <platform/tests.h>

#include <rckid/graphics/bitmap.h>

namespace {
    constexpr uint16_t bmp[] = {
        0xffff, 0xffff, 0xffff, 0xffff, 
        0xffff, 0xff00, 0x0000, 0xffff, 
        0xffff, 0x0000, 0x00ff, 0xffff, 
        0xffff, 0xffff, 0xffff, 0xffff, 
        4, 4
    };

    static_assert(sizeof(bmp) == 18 * 2);
}


TEST(graphics, imageSourceMemory) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    ImageSource img{bmp};
    EXPECT(g_.usedDelta() == 0);
    EXPECT(g_.reservedDelta() == 0);
    EXPECT(img.good());
    EXPECT(img.type() == ImageSource::Type::Memory);
    EXPECT(img.size() == sizeof(bmp));
}