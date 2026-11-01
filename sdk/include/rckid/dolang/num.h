#pragma once

#include <cstdint>

#include <platform.h>
#include <rckid/error.h>


namespace rckid::dolang {

    /** Numeric value representation in doLang.

        All numbers are rational, stored as 32bit numerator and 32bit denominator. The following invariants always hold:

        - the denominator is positive
        - the fraction is in lowest terms (so that equality is simple bitwise comparison)
        - both numerator and denominator are within [-INT32_MAX, INT32_MAX], i.e. INT32_MIN is never used so that negation is always safe

        All intermediate computations are done in 64bits. If the result does not fit into 32/32 bits, the closest representable fraction is used instead (best rational approximation via continued fractions). If the result's magnitude is larger than INT32_MAX, it cannot be represented at all, which is a fatal error for now.
     */
    class num {
    public:

        static constexpr int32_t MAX = INT32_MAX;

        num() = default;

        num(int32_t n) : n_(n), d_(1) {
            // TODO this should be a doLang runtime error once we have them
            ASSERT(n != INT32_MIN);
        }

        num(int64_t n, int64_t d) {
            // TODO this should be a doLang runtime error once we have them
            ASSERT(d != 0);
            bool negative = (n < 0) != (d < 0);
            uint64_t un = abs64(n);
            uint64_t ud = abs64(d);
            // reduce to lowest terms
            uint64_t g = gcd(un, ud);
            un /= g;
            ud /= g;
            if (un > static_cast<uint64_t>(MAX) || ud > static_cast<uint64_t>(MAX))
                approximate(un, ud);
            n_ = negative ? -static_cast<int32_t>(un) : static_cast<int32_t>(un);
            d_ = static_cast<int32_t>(ud);
        }

        num(num const & other) = default;
        num & operator = (num const & other) = default;

        bool isInteger() const { return d_ == 1;}

        int32_t n() const { return n_; }
        int32_t d() const { return d_; }

        // comparison operators (since denominators are always positive, cross multiplication preserves the ordering)

        bool operator == (num const & other) const { return n_ == other.n_ && d_ == other.d_; }
        bool operator != (num const & other) const { return n_ != other.n_ || d_ != other.d_; }
        bool operator < (num const & other) const { return cross(other) < other.cross(*this); }
        bool operator > (num const & other) const { return cross(other) > other.cross(*this); }
        bool operator <= (num const & other) const { return cross(other) <= other.cross(*this); }
        bool operator >= (num const & other) const { return cross(other) >= other.cross(*this); }

        // arithmetic operators
        //
        // Since numerators and denominators are at most INT32_MAX, products fit in 62 bits and sums of two such products in 63 bits, so the intermediate values never overflow int64_t.

        num operator - () const {
            num result;
            result.n_ = -n_;
            result.d_ = d_;
            return result;
        }

        num operator + (num const & other) const {
            if (isInteger() && other.isInteger())
                return fromInt(static_cast<int64_t>(n_) + other.n_);
            return num{cross(other) + other.cross(*this), static_cast<int64_t>(d_) * other.d_};
        }

        num operator - (num const & other) const {
            if (isInteger() && other.isInteger())
                return fromInt(static_cast<int64_t>(n_) - other.n_);
            return num{cross(other) - other.cross(*this), static_cast<int64_t>(d_) * other.d_};
        }

        num operator * (num const & other) const {
            if (isInteger() && other.isInteger())
                return fromInt(static_cast<int64_t>(n_) * other.n_);
            return num{static_cast<int64_t>(n_) * other.n_, static_cast<int64_t>(d_) * other.d_};
        }

        num operator / (num const & other) const {
            // integer division still produces rational, so no fast path here, the constructor normalizes the sign
            return num{cross(other), static_cast<int64_t>(d_) * other.n_};
        }

    private:

        /** Numerator multiplied by the other's denominator in 64 bits.
         */
        int64_t cross(num const & other) const { return static_cast<int64_t>(n_) * other.d_; }

        static num fromInt(int64_t value) {
            // TODO this should be a doLang runtime error once we have them
            ASSERT(value >= -MAX && value <= MAX);
            return num{static_cast<int32_t>(value)};
        }

        static uint64_t abs64(int64_t x) {
            // works for INT64_MIN as well
            return x < 0 ? (~static_cast<uint64_t>(x) + 1) : static_cast<uint64_t>(x);
        }

        static uint64_t gcd(uint64_t a, uint64_t b) {
            while (b != 0) {
                uint64_t t = a % b;
                a = b;
                b = t;
            }
            return a;
        }

        /** Replaces n/d (non-negative, in lowest terms) with the closest fraction whose numerator and denominator are both at most MAX.

            Uses continued fraction expansion, keeping the last convergent that fits within the bounds and then checking whether the best semiconvergent is closer.
         */
        static void approximate(uint64_t & n, uint64_t & d) {
            constexpr uint64_t B = static_cast<uint64_t>(MAX);
            // TODO this should be a doLang runtime error once we have them
            ASSERT(n / d <= B);
            // h0/k0 and h1/k1 are the two previous convergents
            uint64_t h0 = 0, k0 = 1;
            uint64_t h1 = 1, k1 = 0;
            uint64_t p = n, q = d;
            while (true) {
                uint64_t a = p / q;
                // compute largest t <= a such that t * h1 + h0 and t * k1 + k0 are both within bounds, avoiding overflow
                uint64_t t = a;
                if (h1 != 0 && (B - h0) / h1 < t)
                    t = (B - h0) / h1;
                if (k1 != 0 && (B - k0) / k1 < t)
                    t = (B - k0) / k1;
                if (t < a) {
                    // the full convergent does not fit, pick the closer of the last convergent and the best semiconvergent
                    uint64_t hs = t * h1 + h0;
                    uint64_t ks = t * k1 + k0;
                    double x = static_cast<double>(n) / static_cast<double>(d);
                    double errConvergent = static_cast<double>(h1) / static_cast<double>(k1) - x;
                    double errSemi = static_cast<double>(hs) / static_cast<double>(ks) - x;
                    if (errSemi < 0)
                        errSemi = -errSemi;
                    if (errConvergent < 0)
                        errConvergent = -errConvergent;
                    if (errSemi < errConvergent) {
                        n = hs;
                        d = ks;
                    } else {
                        n = h1;
                        d = k1;
                    }
                    return;
                }
                uint64_t h2 = a * h1 + h0;
                uint64_t k2 = a * k1 + k0;
                h0 = h1; k0 = k1;
                h1 = h2; k1 = k2;
                uint64_t r = p - a * q;
                if (r == 0) {
                    // exact, cannot really happen as n/d did not fit, but be safe
                    n = h1;
                    d = k1;
                    return;
                }
                p = q;
                q = r;
            }
        }

        int32_t n_ = 0;
        int32_t d_ = 1;

    }; // dolang::num

    static_assert(sizeof(num) == 8);

}// namespace rckid::dolang
