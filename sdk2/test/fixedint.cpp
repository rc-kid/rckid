#include <platform/tests.h>
#include <rckid/fixedint.h>

using namespace rckid;

TEST(fixedInt, add) {
    EXPECT(FixedInt{1.5f} + FixedInt{1.5f} == FixedInt{3});
}

TEST(fixedInt, divisionByInt) {
    FixedInt x{1};
    EXPECT(x / 2 == FixedInt(0.5f));
    EXPECT(x / FixedInt{2} == FixedInt(0.5f));
    FixedInt y = x / 2;
    EXPECT(y * 2 == 1);
}

TEST(fixedInt, divisionByFixedInt) {
    EXPECT(FixedInt{3} / FixedInt{1.5f} == 2);
    EXPECT(FixedInt{3} / FixedInt{1} == 3);
}

TEST(fixedInt, multiply) {
    EXPECT(FixedInt{2} * FixedInt{3} == FixedInt{6});
    EXPECT(FixedInt{1.5f} * FixedInt{2} == FixedInt{3});
    EXPECT(FixedInt{2} * FixedInt{1.5f} == FixedInt{3});
    // test the range
    EXPECT(FixedInt{320} * FixedInt{320} == FixedInt{102400});
}

TEST(fixedRatio, fromFloat) {
    FixedRatio r{0.5f};
    EXPECT(r >= FixedRatio{0.5f});
    EXPECT(r <= FixedRatio{0.5f});
    EXPECT(r.toInt<16>() == 32767);
    EXPECT(r.toInt<8>() == 128);
    r = 1.0f;
    EXPECT(r.toInt<16>() == 65535);
    EXPECT(r.toInt<8>() == 255);
}