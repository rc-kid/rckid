#pragma once

#include "../rckid.h"

namespace rckid {

    /** RGB color in display's native 565 format.
     
        Although internally the color is 565, the r, g and b elements are reported and can be set in 0..255 range, which is internally converted to the appropriate 0..31 and 0..64 ranges for red & blue and green respectively. 
     */
    PACKED(class alignas(uint16_t) ColorRGB {
    public:

        constexpr static uint8_t BPP = 16;

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

        constexpr bool operator == (ColorRGB const & other) const { return raw_ == other.raw_; }
        constexpr bool operator != (ColorRGB const & other) const { return raw_ != other.raw_; }

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

        constexpr Color256() = default;
        constexpr Color256(uint8_t color): raw_{color} {}

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

        constexpr Color256 operator + (int index) const { return Color256{static_cast<uint8_t>(index + raw_)}; }

    private:
        uint8_t raw_ = 0;

    }); // rckid::Color256

    static_assert(sizeof(Color256) == 1);

    PACKED(class Color16 {
    public:
        static constexpr uint8_t BPP = 4;

        constexpr Color16() = default;
        constexpr Color16(uint8_t color): 
            raw_{color} {
            ASSERT(color < 16);
        }

        constexpr Color16(Color16 const &) = default;
        constexpr Color16 & operator = (Color16 const &) = default;

        constexpr static Color16 fromRaw(uint8_t raw) { return Color16{raw}; }
        constexpr uint8_t toRaw() const { return raw_; }

        constexpr bool operator == (Color16 const & other) { return raw_ == other.raw_; }
        constexpr bool operator != (Color16 const & other) { return raw_ != other.raw_; }

        constexpr Color16 operator + (int index) const { return Color16{static_cast<uint8_t>((index + raw_) & 0x0f)}; }

    private:
        uint8_t raw_ = 0;

    }); // rckid::Color16

    namespace color {

#define COLOR(NAME, R, G, B) constexpr ColorRGB NAME{R, G, B};
#include "colors.inc.h"

    } // rckid::color

    /** Pallete holder, templated by the Color used. 
     
        Defines palette field and getters & setters for color types where palettes are necessary and a dummy palette getter for ColorRGB to satisfy the API needs. Any graphics object that wishes to hold its own palette (such as bitmaps and tile engines) should inherit from this class to conditionally include the palette field.   
     */
    template<typename COLOR>
    class PaletteHolder {
    public:
        ColorRGB const * palette() const {
            return palette_;
        }

        void setPalette(ColorRGB const * value) {
            palette_ = value;
        }
    protected:
        PaletteHolder() = default;
        PaletteHolder(ColorRGB const * palette): palette_{palette} {}

        ColorRGB const * palette_ = nullptr;
    }; 

    /** Specialization for the palette holder for ColorRGB objects where palette is not supported. Only defines a dummy palatte returning function that always returns nullptr.
     */
    template<>
    class PaletteHolder<ColorRGB> {
    public:
        ColorRGB const * palette() const { return nullptr; }
    protected:
        PaletteHolder() = default;
        PaletteHolder(ColorRGB const * palette) {
            ASSERT(palette == nullptr); // Palettes are not supported for ColorRGB graphics
        }
    };

    namespace palette {

        inline void fillPalette16(ColorRGB * p) {
            p[0] = color::Black;
            p[1] = color::DarkRed;
            p[2] = color::DarkGreen;
            p[3] = color::DarkYellow;
            p[4] = color::DarkBlue;
            p[5] = color::DarkViolet;
            p[6] = color::DarkCyan;
            p[7] = color::LightGray;
            p[8] = color::Gray;
            p[9] = color::Red;
            p[10] = color::Green;
            p[11] = color::Yellow;
            p[12] = color::Blue;
            p[13] = color::Violet;
            p[14] = color::Cyan;
            p[15] = color::White;
        }

        inline ColorRGB * generatePalette16() {
            ColorRGB * p = new ColorRGB[16];
            fillPalette16(p);
            return p;
        }

        inline ColorRGB * generatePalette256() {
            ColorRGB * p = new ColorRGB[256];
            fillPalette16(p);
            return p;
        }

    } // rckid::palette

} // namespace rckid