#pragma once

#include <platform.h>
#include <rckid/error.h>

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
        constexpr FixedInt(float value): value_{static_cast<int>(value * static_cast<float>(1 << FRACTION_BITS))} {}
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


    /** Similar to FixedInt, but only for values in range from 0 to 1 with 16bit precision.
     
        This is very useful anywhere a normalized ratio can be used, e.g., for animation progress, alpha blending factors, etc. as it allows fast conversion to integer ranges.
     */
    class FixedRatio {
    public:
        static constexpr uint32_t PRECISION = 16;

        constexpr FixedRatio() = default;
        constexpr FixedRatio(float value): value_{static_cast<uint32_t>(value * ((1 << PRECISION) - 1))} {
            ASSERT(value <= 1.0f && value >= 0.0f);
            ASSERT(value < ((1 << PRECISION) - 1)); // overflow in the above calculation
        }
        constexpr FixedRatio(uint32_t value, uint32_t max): value_{(value * ((1 << PRECISION) - 1)) / max} {
            ASSERT(value <= max);
            ASSERT(value < ((1 << PRECISION) - 1)); // overflow in the above calculation
        }

        constexpr FixedRatio(uint32_t rawValue): value_{rawValue} {
            ASSERT(rawValue <= ((1 << PRECISION) - 1));
        }

        constexpr FixedRatio(FixedRatio const &) = default;

        static FixedRatio Full() { return FixedRatio{static_cast<uint32_t>((1 << PRECISION) - 1)}; }

        static FixedRatio Empty() { return FixedRatio{0U}; }

        FixedRatio & operator = (FixedRatio const &) = default;

        bool operator == (FixedRatio other) const { return value_ == other.value_; }
        bool operator != (FixedRatio other) const { return value_ != other.value_; }
        bool operator < (FixedRatio other) const { return value_ < other.value_; }
        bool operator <= (FixedRatio other) const { return value_ <= other.value_; }
        bool operator > (FixedRatio other) const { return value_ > other.value_; }
        bool operator >= (FixedRatio other) const { return value_ >= other.value_; }

        /** Converts the float ratio to an integer with given bit precision.
         
            The precision value must be no larger than that of the FixedRatio (16 bits). The conversion rounds the value to the nearest integer and clamps it to the maximum representable value.
         */
        template<size_t BITS>
        uint32_t toInt() const {
            static_assert(BITS <= PRECISION);
            uint32_t cutoff = PRECISION - BITS;
            if (cutoff == 0)
                return value_;
            uint32_t result = value_ + (1 << (cutoff - 1));
            result = result >> cutoff;
            // and clamp the result
            if (result >= (1 << BITS))
                result = (1 << BITS) - 1;
            return result;
        }

        /** Returns the raw fixed-point value, which is number between 0 and 65535.
         
            This is almost *never* what you want and the toInt<>() method should be used instead.
         */
        uint32_t raw() const { return value_; }

    private:
        uint32_t value_ = 0;

    }; // rckid::FixedRatio

    inline int32_t operator * (FixedRatio ratio, int32_t value) {
        return (value * ratio.raw() + 32767) >> 16;
    }

} // namespace rckid