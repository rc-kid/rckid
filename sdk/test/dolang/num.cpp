#include <platform/tests.h>
#include <rckid/dolang/num.h>

using namespace rckid;
using namespace rckid::dolang;

namespace {
    double toDouble(Num x) { return static_cast<double>(x.n()) / static_cast<double>(x.d()); }

    double absDiff(double a, double b) { return a > b ? a - b : b - a; }

    bool isValid(Num x) {
        return x.d() > 0 && x.d() <= Num::MAX && x.n() >= -Num::MAX && x.n() <= Num::MAX;
    }
}

TEST(dolangNum, defaultIsZero) {
    Num x;
    EXPECT(x.n() == 0);
    EXPECT(x.d() == 1);
    EXPECT(x.isInteger());
}

TEST(dolangNum, construction) {
    Num x{1, 2};
    EXPECT(x.n() == 1);
    EXPECT(x.d() == 2);
    EXPECT(!x.isInteger());
    // integers
    EXPECT(Num{5}.isInteger());
    EXPECT(Num{-5}.n() == -5);
    EXPECT(Num{6, 3}.isInteger());
    EXPECT(Num{6, 3} == Num{2});
}

TEST(dolangNum, lowestTerms) {
    EXPECT(Num{2, 4} == Num{1, 2});
    EXPECT(Num{2, 4}.n() == 1);
    EXPECT(Num{2, 4}.d() == 2);
    EXPECT(Num{12, 18} == Num{2, 3});
    EXPECT(Num{-12, 18} == Num{-2, 3});
    EXPECT(Num{17, 13}.n() == 17);
    EXPECT(Num{17, 13}.d() == 13);
    // zero is always 0/1
    EXPECT(Num{0, 5}.n() == 0);
    EXPECT(Num{0, 5}.d() == 1);
    EXPECT(Num{0, -5} == Num{0});
}

TEST(dolangNum, positiveDenominator) {
    EXPECT(Num{1, -2}.n() == -1);
    EXPECT(Num{1, -2}.d() == 2);
    EXPECT(Num{-1, -2} == Num{1, 2});
    EXPECT(Num{-1, 2} == Num{1, -2});
}

TEST(dolangNum, reductionBeforeRangeCheck) {
    // Numerator and denominator out of 32bit range, but the reduced fraction fits exactly
    Num x{3000000000LL, 2000000000LL};
    EXPECT(x == Num{3, 2});
    Num y{-6000000000LL, 4000000000LL};
    EXPECT(y == Num{-3, 2});
}

TEST(dolangNum, negation) {
    EXPECT(-Num{1, 2} == Num{-1, 2});
    EXPECT(-Num{-3} == Num{3});
    EXPECT(-Num{} == Num{});
    EXPECT(-Num{Num::MAX} == Num{-Num::MAX});
}

TEST(dolangNum, addition) {
    EXPECT(Num{1} + Num{2} == Num{3});
    EXPECT(Num{1, 2} + Num{1, 3} == Num{5, 6});
    EXPECT(Num{1, 2} + Num{1, 2} == Num{1});
    EXPECT((Num{1, 2} + Num{1, 2}).isInteger());
    EXPECT(Num{1} + Num{3, 4} == Num{7, 4});
    EXPECT(Num{-1, 2} + Num{1, 2} == Num{0});
}

TEST(dolangNum, subtraction) {
    EXPECT(Num{5} - Num{7} == Num{-2});
    EXPECT(Num{1, 2} - Num{1, 3} == Num{1, 6});
    EXPECT(Num{1, 3} - Num{1, 2} == Num{-1, 6});
    EXPECT(Num{3, 4} - Num{3, 4} == Num{0});
}

TEST(dolangNum, multiplication) {
    EXPECT(Num{6} * Num{7} == Num{42});
    EXPECT(Num{2, 3} * Num{3, 4} == Num{1, 2});
    EXPECT(Num{-2, 3} * Num{3} == Num{-2});
    EXPECT(Num{-2, 3} * Num{-3, 2} == Num{1});
    EXPECT(Num{0} * Num{5, 7} == Num{0});
}

TEST(dolangNum, division) {
    // division of integers produces rationals, not truncated integers
    EXPECT(Num{1} / Num{2} == Num{1, 2});
    EXPECT(Num{6} / Num{3} == Num{2});
    EXPECT(Num{3} / Num{-6} == Num{-1, 2});
    EXPECT(Num{1, 2} / Num{1, 4} == Num{2});
    EXPECT(Num{2, 3} / Num{-4, 9} == Num{-3, 2});
    // division by a fraction (x / 3/4 is x / (3/4) when constructed explicitly)
    EXPECT(Num{3} / Num{3, 4} == Num{4});
}

TEST(dolangNum, comparison) {
    EXPECT(Num{1, 3} < Num{1, 2});
    EXPECT(Num{1, 2} > Num{1, 3});
    EXPECT(Num{-1, 2} < Num{-1, 3});
    EXPECT(Num{-1, 2} < Num{1, 3});
    EXPECT(Num{1, 2} <= Num{1, 2});
    EXPECT(Num{1, 2} >= Num{1, 2});
    EXPECT(Num{1, 2} != Num{1, 3});
    EXPECT(!(Num{1, 2} < Num{1, 2}));
    EXPECT(Num{2} > Num{3, 2});
}

TEST(dolangNum, comparisonLargeValues) {
    // cross products overflow 32 bits
    EXPECT(Num{100000, 3} > Num{200000, 7});
    EXPECT(Num{200000, 7} < Num{100000, 3});
    EXPECT(Num{Num::MAX, 2} > Num{Num::MAX - 1, 2});
    EXPECT(Num{Num::MAX - 1, Num::MAX} < Num{1});
    EXPECT(Num{-Num::MAX, Num::MAX - 1} < Num{-1});
}

TEST(dolangNum, arithmeticLargeValues) {
    // intermediate values overflow 32 bits, but the results fit
    EXPECT(Num{100000} * Num{20000} / Num{40000} == Num{50000});
    EXPECT(Num{Num::MAX} - Num{Num::MAX} == Num{0});
    EXPECT(Num{Num::MAX, 2} + Num{Num::MAX, 2} == Num{Num::MAX});
    EXPECT(Num{65536, 3} * Num{3, 65536} == Num{1});
}

TEST(dolangNum, approximationDenominatorOverflow) {
    // 65537 * 65539 does not fit in 32 bits, result must be approximated
    // (expected values are the best approximations as computed by Python's Fraction.limit_denominator)
    Num x = Num{1, 65537} + Num{1, 65539};
    EXPECT(isValid(x));
    EXPECT(x == Num{1, 32769});
    // tiny values
    Num y = Num{1, 46341} * Num{1, 46341};
    EXPECT(isValid(y));
    EXPECT(y == Num{1, Num::MAX});
}

TEST(dolangNum, approximationPi) {
    Num pi{314159265358979LL, 100000000000000LL};
    EXPECT(isValid(pi));
    EXPECT(absDiff(toDouble(pi), 3.14159265358979) < 1e-15);
    Num negPi{-314159265358979LL, 100000000000000LL};
    EXPECT(negPi == -pi);
}

TEST(dolangNum, approximationIsBestCandidate) {
    // 1/3 shifted by a tiny amount does not fit, the best approximation within the bounds is 1/3 itself
    Num x{1000000000001LL, 3000000000000LL};
    EXPECT(isValid(x));
    EXPECT(x == Num{1, 3});
}

TEST(dolangNum, harmonicSeriesStaysValid) {
    // denominators explode quickly, the values must stay valid and close to the floating point result
    Num x;
    double expected = 0;
    for (int i = 1; i <= 1000; ++i) {
        x = x + Num{1, i};
        expected += 1.0 / i;
        if (!isValid(x))
            break;
    }
    EXPECT(isValid(x));
    EXPECT(absDiff(toDouble(x), expected) < 1e-6);
}
