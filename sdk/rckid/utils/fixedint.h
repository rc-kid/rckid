#pragma once

#include <cstdint>

#include "../graphics/geometry.h"

namespace rckid {

    /** Fixed-point integer value.

        Uses 24 bits for the integer part and 8 bits for the fraction part. 
     */
    class FixedInt {
    public:

        static constexpr uint8_t FRACTION_BITS = 4;
        static constexpr uint8_t FRACTION_HALF = 1 << (FRACTION_BITS - 1);

        FixedInt() = default;
        explicit constexpr FixedInt(int value): value_{value << FRACTION_BITS} {}
        constexpr FixedInt(int value, uint8_t fraction): value_{(value << FRACTION_BITS) + fraction} {}
        constexpr FixedInt(FixedInt const &) = default;

        FixedInt & operator = (int other) { value_ = (other << FRACTION_BITS); return *this; }

        FixedInt operator + (FixedInt other) { 
            FixedInt result;
            result.value_ = value_ + other.value_;
            return result;
        }

        FixedInt operator - (FixedInt other) { 
            FixedInt result;
            result.value_ = value_ - other.value_;
            return result;
        }

        FixedInt operator * (FixedInt other) {
            FixedInt result;
            result.value_ = (value_ * other.value_) >> FRACTION_BITS;
            return result;
        }

        FixedInt operator + (int other) { 
            FixedInt result;
            result.value_ = value_ + (other << FRACTION_BITS);
            return result;
        }

        FixedInt operator - (int other) { 
            FixedInt result;
            result.value_ = value_ - (other << FRACTION_BITS);
            return result;
        }

        FixedInt operator * (int other) {
            FixedInt result;
            result.value_ = value_ * other;
            return result;
        }

        FixedInt operator / (int other) {
            FixedInt result;
            result.value_ = value_ / other;
            return result;
        }

        FixedInt operator / (FixedInt other) {
            FixedInt result;
            result.value_ = static_cast<int>((static_cast<int64_t>(value_) << FRACTION_BITS) / other.value_);
            return result;
        }

        FixedInt & operator += (FixedInt other) {
            value_ = value_ + other.value_;
            return *this;
        }


        FixedInt & operator -= (FixedInt other) {
            value_ = value_ - other.value_;
            return *this;
        }

        FixedInt & operator *= (FixedInt other) {
            value_ = (value_ * other.value_) >> FRACTION_BITS;
            return *this;
        }

        FixedInt & operator +=(int other) { return *this += FixedInt{other}; }
        FixedInt & operator -=(int other) { return *this -= FixedInt{other}; }
        FixedInt & operator *=(int other) { return *this *= FixedInt{other}; }

        FixedInt clamp(FixedInt min, FixedInt max) {
            FixedInt result{*this};
            if (result.value_ < min.value_)
                result.value_ = min.value_;
            if (result.value_ > max.value_)
                result.value_ = max.value_;
            return result;
        }

        FixedInt clamp(int min, int max) { return clamp(FixedInt{min}, FixedInt{max}); }

        bool operator == (FixedInt other) const { return value_ == other.value_; }
        bool operator != (FixedInt other) const { return value_ != other.value_; }
        bool operator < (FixedInt other) const { return value_ < other.value_; }
        bool operator <= (FixedInt other) const { return value_ <= other.value_; }
        bool operator > (FixedInt other) const { return value_ > other.value_; }
        bool operator >= (FixedInt other) const { return value_ >= other.value_; }

        bool inRange(FixedInt min, FixedInt max) {
            return value_ >= min.value_ && value_ <= max.value_;
        }

        /** Implicit conversion from the fixed int to int with rounding. 
         */
        operator int () const { return round(); }

        int clip() const { 
            return (value_ >> FRACTION_BITS);
        }
        int round() const {
            return ((value_ >= 0) ? (value_ + FRACTION_HALF) : (value_ - FRACTION_HALF)) >> FRACTION_BITS;
        }

        int raw() const { return value_; }

    private:


        int value_ = 0;

    }; // rckid::FixedInt

    using FixedPoint = TPoint<FixedInt>;

    inline FixedPoint operator + (FixedPoint p, Point other) { 
        return FixedPoint{p.x + other.x, p.y + other.y}; 
    }

    inline FixedPoint operator - (FixedPoint p, Point other) { 
        return FixedPoint{p.x - other.x, p.y - other.y}; 
    }

    inline FixedPoint operator * (FixedPoint p, int other) { 
        return FixedPoint{p.x * other, p.y * other}; 
    }
}

inline rckid::FixedInt operator "" _fi(unsigned long long value) { 
    return rckid::FixedInt{static_cast<int>(value)}; 
}