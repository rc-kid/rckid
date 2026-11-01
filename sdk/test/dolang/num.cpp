#include <platform/tests.h>
#include <rckid/dolang/num.h>

using namespace rckid;
using namespace rckid::dolang;

namespace {
    double toDouble(num x) { return static_cast<double>(x.n()) / static_cast<double>(x.d()); }

    double absDiff(double a, double b) { return a > b ? a - b : b - a; }

    bool isValid(num x) {
        return x.d() > 0 && x.d() <= num::MAX && x.n() >= -num::MAX && x.n() <= num::MAX;
    }
}

TEST(dolangNum, defaultIsZero) {
    num x;
    EXPECT(x.n() == 0);
    EXPECT(x.d() == 1);
    EXPECT(x.isInteger());
}

TEST(dolangNum, construction) {
    num x{1, 2};
    EXPECT(x.n() == 1);
    EXPECT(x.d() == 2);
    EXPECT(!x.isInteger());
    // integers
    EXPECT(num{5}.isInteger());
    EXPECT(num{-5}.n() == -5);
    EXPECT(num{6, 3}.isInteger());
    EXPECT(num{6, 3} == num{2});
}

TEST(dolangNum, lowestTerms) {
    EXPECT(num{2, 4} == num{1, 2});
    EXPECT(num{2, 4}.n() == 1);
    EXPECT(num{2, 4}.d() == 2);
    EXPECT(num{12, 18} == num{2, 3});
    EXPECT(num{-12, 18} == num{-2, 3});
    EXPECT(num{17, 13}.n() == 17);
    EXPECT(num{17, 13}.d() == 13);
    // zero is always 0/1
    EXPECT(num{0, 5}.n() == 0);
    EXPECT(num{0, 5}.d() == 1);
    EXPECT(num{0, -5} == num{0});
}

TEST(dolangNum, positiveDenominator) {
    EXPECT(num{1, -2}.n() == -1);
    EXPECT(num{1, -2}.d() == 2);
    EXPECT(num{-1, -2} == num{1, 2});
    EXPECT(num{-1, 2} == num{1, -2});
}

TEST(dolangNum, reductionBeforeRangeCheck) {
    // numerator and denominator out of 32bit range, but the reduced fraction fits exactly
    num x{3000000000LL, 2000000000LL};
    EXPECT(x == num{3, 2});
    num y{-6000000000LL, 4000000000LL};
    EXPECT(y == num{-3, 2});
}

TEST(dolangNum, negation) {
    EXPECT(-num{1, 2} == num{-1, 2});
    EXPECT(-num{-3} == num{3});
    EXPECT(-num{} == num{});
    EXPECT(-num{num::MAX} == num{-num::MAX});
}

TEST(dolangNum, addition) {
    EXPECT(num{1} + num{2} == num{3});
    EXPECT(num{1, 2} + num{1, 3} == num{5, 6});
    EXPECT(num{1, 2} + num{1, 2} == num{1});
    EXPECT((num{1, 2} + num{1, 2}).isInteger());
    EXPECT(num{1} + num{3, 4} == num{7, 4});
    EXPECT(num{-1, 2} + num{1, 2} == num{0});
}

TEST(dolangNum, subtraction) {
    EXPECT(num{5} - num{7} == num{-2});
    EXPECT(num{1, 2} - num{1, 3} == num{1, 6});
    EXPECT(num{1, 3} - num{1, 2} == num{-1, 6});
    EXPECT(num{3, 4} - num{3, 4} == num{0});
}

TEST(dolangNum, multiplication) {
    EXPECT(num{6} * num{7} == num{42});
    EXPECT(num{2, 3} * num{3, 4} == num{1, 2});
    EXPECT(num{-2, 3} * num{3} == num{-2});
    EXPECT(num{-2, 3} * num{-3, 2} == num{1});
    EXPECT(num{0} * num{5, 7} == num{0});
}

TEST(dolangNum, division) {
    // division of integers produces rationals, not truncated integers
    EXPECT(num{1} / num{2} == num{1, 2});
    EXPECT(num{6} / num{3} == num{2});
    EXPECT(num{3} / num{-6} == num{-1, 2});
    EXPECT(num{1, 2} / num{1, 4} == num{2});
    EXPECT(num{2, 3} / num{-4, 9} == num{-3, 2});
    // division by a fraction (x / 3/4 is x / (3/4) when constructed explicitly)
    EXPECT(num{3} / num{3, 4} == num{4});
}

TEST(dolangNum, comparison) {
    EXPECT(num{1, 3} < num{1, 2});
    EXPECT(num{1, 2} > num{1, 3});
    EXPECT(num{-1, 2} < num{-1, 3});
    EXPECT(num{-1, 2} < num{1, 3});
    EXPECT(num{1, 2} <= num{1, 2});
    EXPECT(num{1, 2} >= num{1, 2});
    EXPECT(num{1, 2} != num{1, 3});
    EXPECT(!(num{1, 2} < num{1, 2}));
    EXPECT(num{2} > num{3, 2});
}

TEST(dolangNum, comparisonLargeValues) {
    // cross products overflow 32 bits
    EXPECT(num{100000, 3} > num{200000, 7});
    EXPECT(num{200000, 7} < num{100000, 3});
    EXPECT(num{num::MAX, 2} > num{num::MAX - 1, 2});
    EXPECT(num{num::MAX - 1, num::MAX} < num{1});
    EXPECT(num{-num::MAX, num::MAX - 1} < num{-1});
}

TEST(dolangNum, arithmeticLargeValues) {
    // intermediate values overflow 32 bits, but the results fit
    EXPECT(num{100000} * num{20000} / num{40000} == num{50000});
    EXPECT(num{num::MAX} - num{num::MAX} == num{0});
    EXPECT(num{num::MAX, 2} + num{num::MAX, 2} == num{num::MAX});
    EXPECT(num{65536, 3} * num{3, 65536} == num{1});
}

TEST(dolangNum, approximationDenominatorOverflow) {
    // 65537 * 65539 does not fit in 32 bits, result must be approximated
    // (expected values are the best approximations as computed by Python's Fraction.limit_denominator)
    num x = num{1, 65537} + num{1, 65539};
    EXPECT(isValid(x));
    EXPECT(x == num{1, 32769});
    // tiny values
    num y = num{1, 46341} * num{1, 46341};
    EXPECT(isValid(y));
    EXPECT(y == num{1, num::MAX});
}

TEST(dolangNum, approximationPi) {
    num pi{314159265358979LL, 100000000000000LL};
    EXPECT(isValid(pi));
    EXPECT(absDiff(toDouble(pi), 3.14159265358979) < 1e-15);
    num negPi{-314159265358979LL, 100000000000000LL};
    EXPECT(negPi == -pi);
}

TEST(dolangNum, approximationIsBestCandidate) {
    // 1/3 shifted by a tiny amount does not fit, the best approximation within the bounds is 1/3 itself
    num x{1000000000001LL, 3000000000000LL};
    EXPECT(isValid(x));
    EXPECT(x == num{1, 3});
}

TEST(dolangNum, harmonicSeriesStaysValid) {
    // denominators explode quickly, the values must stay valid and close to the floating point result
    num x;
    double expected = 0;
    for (int i = 1; i <= 1000; ++i) {
        x = x + num{1, i};
        expected += 1.0 / i;
        if (!isValid(x))
            break;
    }
    EXPECT(isValid(x));
    EXPECT(absDiff(toDouble(x), expected) < 1e-6);
}
