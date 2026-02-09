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