#pragma once

#include <cstdint>

#include "../graphics/geometry.h"

namespace rckid {

    /** Fixed-point integer value.

        Uses 28 bits for the integer part and 4 bits for the fraction part. This gives us 24bit integer range even for multiplication.
     */
    class FixedInt {
    public:

        static constexpr uint8_t FRACTION_BITS = 4;
        static constexpr uint8_t FRACTION_HALF = 1 << (FRACTION_BITS - 1);

        constexpr FixedInt() = default;
        constexpr FixedInt(int value): value_{value << FRACTION_BITS} {}
        constexpr FixedInt(int value, uint32_t fraction): value_{static_cast<int>((value << FRACTION_BITS) + fraction)} {}
        constexpr FixedInt(FixedInt const &) = default;

        constexpr FixedInt operator + (FixedInt other) const { 
            FixedInt result;
            result.value_ = value_ + other.value_;
            return result;
        }

        constexpr FixedInt operator - (FixedInt other) const { 
            FixedInt result;
            result.value_ = value_ - other.value_;
            return result;
        }

        constexpr FixedInt operator * (FixedInt other) const {
            FixedInt result;
            result.value_ = (value_ * other.value_) >> FRACTION_BITS;
            return result;
        }

        constexpr FixedInt operator / (FixedInt other) const {
            FixedInt result;
            result.value_ = static_cast<int>((static_cast<int64_t>(value_) << FRACTION_BITS) / other.value_);
            return result;
        }

        constexpr FixedInt & operator += (FixedInt other) {
            value_ = value_ + other.value_;
            return *this;
        }


        constexpr FixedInt & operator -= (FixedInt other) {
            value_ = value_ - other.value_;
            return *this;
        }

        constexpr FixedInt & operator *= (FixedInt other) {
            value_ = (value_ * other.value_) >> FRACTION_BITS;
            return *this;
        }

        constexpr FixedInt clamp(FixedInt min, FixedInt max) {
            FixedInt result{*this};
            if (result.value_ < min.value_)
                result.value_ = min.value_;
            if (result.value_ > max.value_)
                result.value_ = max.value_;
            return result;
        }

        constexpr bool operator == (FixedInt other) const { return value_ == other.value_; }
        constexpr bool operator != (FixedInt other) const { return value_ != other.value_; }
        constexpr bool operator < (FixedInt other) const { return value_ < other.value_; }
        constexpr bool operator <= (FixedInt other) const { return value_ <= other.value_; }
        constexpr bool operator > (FixedInt other) const { return value_ > other.value_; }
        constexpr bool operator >= (FixedInt other) const { return value_ >= other.value_; }

        constexpr bool inRange(FixedInt min, FixedInt max) {
            return value_ >= min.value_ && value_ <= max.value_;
        }

        constexpr int32_t clip() const { 
            return (value_ >> FRACTION_BITS);
        }

        constexpr int32_t round() const {
            return ((value_ >= 0) ? (value_ + FRACTION_HALF) : (value_ - FRACTION_HALF)) >> FRACTION_BITS;
        }

        constexpr int32_t raw() const { return value_; }

    private:

        int32_t value_ = 0;

    }; // rckid::FixedInt

    using FixedPoint = TPoint<FixedInt>;


    inline constexpr FixedInt operator + (int a, FixedInt b) { return FixedInt{a} + b; }
    inline constexpr FixedInt operator - (int a, FixedInt b) { return FixedInt{a} - b; }
    inline constexpr FixedInt operator * (int a, FixedInt b) { return FixedInt{a} * b; }
    inline constexpr FixedInt operator / (int a, FixedInt b) { return FixedInt{a} / b; }

    inline constexpr bool operator > (int a, FixedInt b) { return FixedInt{a} > b; }
    inline constexpr bool operator >= (int a, FixedInt b) { return FixedInt{a} >= b; }
    inline constexpr bool operator < (int a, FixedInt b) { return FixedInt{a} < b; }
    inline constexpr bool operator <= (int a, FixedInt b) { return FixedInt{a} <= b; }

    inline FixedPoint operator + (FixedPoint p, Point other) { 
        return FixedPoint{p.x + other.x, p.y + other.y}; 
    }

    inline FixedPoint operator - (FixedPoint p, Point other) { 
        return FixedPoint{p.x - other.x, p.y - other.y}; 
    }

    inline FixedPoint operator * (FixedPoint p, int other) { 
        return FixedPoint{p.x * other, p.y * other}; 
    }

    inline Writer & operator << (Writer & w, FixedInt const & f) {
        w << f.clip();
        return w;
    }
}
