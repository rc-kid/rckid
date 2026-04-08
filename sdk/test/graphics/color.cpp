#include <platform/tests.h>

#include <rckid/string.h>
#include <rckid/graphics/color.h>

TEST(color, toString) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    Color c = Color::RGB(123, 45, 67);
    String s = STR(c);
    EXPECT(s == "#7b2d43");
}

TEST(color, blend) {
    using namespace rckid;
    Color a = Color::RGB(0, 255, 0);
    Color b = Color::RGB(255, 0, 128);
    EXPECT(Color::blend(a, b, 0) == a);
    EXPECT(Color::blend(a, b, 255) == b);
    EXPECT(Color::blend(a, b, 128) == Color::RGB(128, 127, 64));
}