#pragma once

#include <platform.h>

namespace rckid {

    /** Fixed-point integer value.

        Uses 28 bits for the integer part and 4 bits for the fraction part. Compared to other fixed point arithmetics, this is very low on the fractional part precision, but still acceptable for UI on RCKid (relatively fast animations). On the plus side, this gives us 32bit multiplication with decent range, keeping it a single instruction on RP2350.
     */
    class FixedInt {
    public:
        static constexpr uint8_t FRACTION_BITS = 4;
        static constexpr uint32_t FRACTION_HALF = 1 << (FRACTION_BITS - 1);

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

        constexpr FixedInt clamp(FixedInt min, FixedInt max) const {
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

        constexpr bool inRange(FixedInt min, FixedInt max) const {
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

    inline constexpr FixedInt operator + (int a, FixedInt b) { return FixedInt{a} + b; }
    inline constexpr FixedInt operator - (int a, FixedInt b) { return FixedInt{a} - b; }
    inline constexpr FixedInt operator * (int a, FixedInt b) { return FixedInt{a} * b; }
    inline constexpr FixedInt operator / (int a, FixedInt b) { return FixedInt{a} / b; }

    inline constexpr bool operator > (int a, FixedInt b) { return FixedInt{a} > b; }
    inline constexpr bool operator >= (int a, FixedInt b) { return FixedInt{a} >= b; }
    inline constexpr bool operator < (int a, FixedInt b) { return FixedInt{a} < b; }
    inline constexpr bool operator <= (int a, FixedInt b) { return FixedInt{a} <= b; }

} // namespace rckid