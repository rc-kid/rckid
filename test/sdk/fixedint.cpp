#include <platform/tests.h>
#include <rckid/utils/fixedint.h>

using namespace rckid;

TEST(fixedInt, add) {
    EXPECT(FixedInt{1, 8} + FixedInt{1, 8} == FixedInt{3});
}

TEST(fixedInt, divisionByInt) {
    FixedInt x{1};
    EXPECT(x / 2 == FixedInt(0, 8));
    EXPECT(x / FixedInt{2} == FixedInt(0, 8));
    FixedInt y = x / 2;
    EXPECT(y * 2 == 1);
}

TEST(fixedInt, divisionByFixedInt) {
    EXPECT(FixedInt{3} / FixedInt{1, 8} == 2);
    EXPECT(FixedInt{3} / FixedInt{1} == 3);
}