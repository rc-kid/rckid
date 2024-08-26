#pragma once

#include "../rckid.h"

namespace rckid {

    /** RGB color in display's native 565 format.
     
        Although internally the color is 565, the r, g and b elements are reported and can be set in 0..255 range, which is internally converted to the appropriate 0..31 and 0..64 ranges for red & blue and green respectively. 
     */
    PACKED(class alignas(uint16_t) ColorRGB {
    public:

        constexpr static uint8_t BPP = 16;

        using RawBufferType = uint16_t *;

        constexpr ColorRGB() = default;

        constexpr ColorRGB(uint8_t r, uint8_t g, uint8_t b): raw_{0} {
            setR(r);
            setG(g);
            setB(b);
        }

        constexpr ColorRGB(ColorRGB const &) = default;
        constexpr ColorRGB & operator = (ColorRGB const &) = default;

        constexpr static ColorRGB fromRaw(uint16_t raw) { return ColorRGB{raw}; };
        constexpr uint16_t toRaw() const { return raw_; }

        constexpr uint8_t r() const { return ((raw_ >> 11) & 0xff) << 3; }
        constexpr uint8_t g() const { return ((raw_ >> 5) & 0x3f) << 2; }
        constexpr uint8_t b() const { return (raw_ & 0xff) << 3; }

        constexpr void setR(uint8_t v) { uint16_t x = v >>= 3; raw_ = (raw_ & 0x07ff) | (x << 11); }
        constexpr void setG(uint8_t v) { uint16_t x = v >>= 2; raw_ = (raw_ & 0xf81f) | (x << 5); }
        constexpr void setB(uint8_t v) { raw_ = (raw_ & 0xffe0) | (v >> 3); }

        constexpr ColorRGB withAlpha(uint8_t alpha) {
            return ColorRGB{
                static_cast<uint8_t>(std::min(255, (r() * alpha + 128) >> 8)),
                static_cast<uint8_t>(std::min(255, (g() * alpha + 128) >> 8)),
                static_cast<uint8_t>(std::min(255, (b() * alpha + 128) >> 8)),
            };
        }

    private:
        constexpr explicit ColorRGB(uint16_t raw): raw_{raw} {}

        uint16_t raw_ = 0;
    }); // rckid::ColorRGB

    static_assert(sizeof(ColorRGB) == 2);

    /** Color index from palette of 256 colors (or less) 
     */
    PACKED(class Color256 {
    public:
        constexpr static  uint8_t BPP = 8;

        using RawBufferType = uint8_t *;

        constexpr Color256() = default;
        constexpr explicit Color256(uint8_t color): raw_{color} {}

        constexpr Color256(Color256 const &) = default;
        constexpr Color256 & operator = (Color256 const &) = default;

        constexpr Color256 & operator = (uint8_t color) {
            raw_ = color;
            return *this;
        }

        constexpr static Color256 fromRaw(uint8_t raw) { return Color256{raw}; }
        constexpr uint8_t toRaw() const { return raw_; }


        constexpr bool operator == (Color256 const & other) { return raw_ == other.raw_; }
        constexpr bool operator != (Color256 const & other) { return raw_ != other.raw_; }

    private:
        uint8_t raw_ = 0;

    }); // rckid::Color256

    static_assert(sizeof(Color256) == 1);

    PACKED(class Color16 {
    public:
        static constexpr uint8_t BPP = 4;

        using RawBufferType = uint8_t *;

        constexpr Color16() = default;
        explicit constexpr Color16(uint8_t color): 
            raw_{color} {
            ASSERT(color < 16);
        }

        constexpr Color16(Color16 const &) = default;
        constexpr Color16 & operator = (Color16 const &) = default;

        constexpr static Color16 fromRaw(uint8_t raw) { return Color16{raw}; }
        constexpr uint8_t toRaw() const { return raw_; }

        constexpr bool operator == (Color16 const & other) { return raw_ == other.raw_; }
        constexpr bool operator != (Color16 const & other) { return raw_ != other.raw_; }

    private:
        uint8_t raw_ = 0;

    }); // rckid::Color16




    namespace color {

#define COLOR(NAME, R, G, B) constexpr ColorRGB NAME{R, G, B};
#include "colors.inc.h"

    } // rckid::color

} // namespace rckid