#pragma once

#include <cstdint>

#include "../graphics/geometry.h"

namespace rckid {

    /** Fixed-point integer value.

        Uses 24 bits for the integer part and 8 bits for the fraction part. 
     */
    class FixedInt {
    public:

        FixedInt() = default;
        constexpr FixedInt(int value): value_{value << 8} {}
        constexpr FixedInt(int value, uint8_t fraction): value_{(value << 8) + fraction} {}
        constexpr FixedInt(FixedInt const &) = default;

        FixedInt & operator = (int other) { value_ = (other << 8); return *this; }

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
            result.value_ = (value_ * other.value_) >> 8;
            return result;
        }

        FixedInt operator + (int other) { 
            FixedInt result;
            result.value_ = value_ + (other << 8);
            return result;
        }

        FixedInt operator - (int other) { 
            FixedInt result;
            result.value_ = value_ - (other << 8);
            return result;
        }

        FixedInt operator * (int other) {
            FixedInt result;
            result.value_ = value_ * other;
            return result;
        }

        FixedInt & operator += (FixedInt other) {
            value_ = value_ + other.value_;
            return *this;
        }

        FixedInt & operator *= (FixedInt other) {
            value_ = (value_ * other.value_) >> 8;
            return *this;
        }



        void clipInRange(FixedInt min, FixedInt max) {
            if (value_ < min.value_)
                value_ = min.value_;
            if (value_ > max.value_)
                value_ = max.value_;
        }

        bool inRange(FixedInt min, FixedInt max) {
            return value_ >= min.value_ && value_ <= max.value_;
        }

        /** Implicit conversion from the fixed int to int with rounding. 
         */
        operator int () const {
            return ((value_ >= 0) ? (value_ + 0x80) : (value_ - 0x80)) >> 8;
        }

    private:


        int value_;

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