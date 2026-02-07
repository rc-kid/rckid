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
    EXPECT(r.toInt<16>() == 32768);
    EXPECT(r.toInt<8>() == 128);
    r = 1.0f;
    EXPECT(r.toInt<16>() == 65536);
    EXPECT(r.toInt<8>() == 256);
}

TEST(fixedRatio, arithmetic) {
    EXPECT(FixedRatio{0.5f} + FixedRatio{0.25f} == FixedRatio{0.75f});
    EXPECT(FixedRatio{0.5f} - FixedRatio{0.25f} == FixedRatio{0.25f});
    EXPECT(FixedRatio{0.5f} * FixedRatio{0.5f} == FixedRatio{0.25f});
    EXPECT(FixedRatio{1.0f} * FixedRatio{1.0f} == FixedRatio{1.0f});
}

TEST(fixedRatio, scale) {
    EXPECT(FixedRatio{0.5f}.scale(100) == 50);
    EXPECT(FixedRatio{0.25f}.scale(100) == 25);
    EXPECT(FixedRatio{0.75f}.scale(100) == 75);
    EXPECT(FixedRatio{0.5f}.scale(-100) == -50);
    EXPECT(FixedRatio{0.25f}.scale(-100) == -25);
    EXPECT(FixedRatio{0.75f}.scale(-100) == -75);
}