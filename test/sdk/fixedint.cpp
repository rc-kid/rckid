#include <platform/tests.h>
#include <rckid/utils/fixedint.h>

using namespace rckid;

TEST(fixedInt, add) {
    EXPECT(FixedInt{1, 128} + FixedInt{1, 128} == FixedInt{3});
}

TEST(fixedInt, divisionByInt) {
    FixedInt x{1};
    EXPECT(x / 2 == FixedInt(0, 128));
    EXPECT(x / FixedInt{2} == FixedInt(0, 128));
    FixedInt y = x / 2;
    EXPECT(y * 2 == 1);
}

TEST(fixedInt, divisionByFixedInt) {
    EXPECT(FixedInt{3} / FixedInt{1, 128} == 2);
    EXPECT(FixedInt{3} / FixedInt{1} == 3);
}