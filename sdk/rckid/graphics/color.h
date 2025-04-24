#pragma once

#include <array>
#include <platform.h>

// on windows, there is RGB macro, which clases with the 565 and 332 colors
#ifdef RGB
#undef RGB
#endif

namespace rckid {

    /** RGB color.
     
        Uses 24bit RGB with 8 bits per channel and provides conversion to and from RGB 565 and RGB332 variants for 16 and 8 bpp pixels. 
     */
    PACKED(class ColorRGB {
    public:

        static constexpr uint8_t BPP = 16;
        static constexpr bool PALETTE = false;

        constexpr ColorRGB() = default;

        constexpr ColorRGB(int r, int g, int b): raw_{ static_cast<uint32_t>((r << 16) | (g << 8) | b) } {
        }

        static constexpr ColorRGB RGB(int r, int g, int b) {
            return ColorRGB(r, g, b);
        }

        static constexpr ColorRGB fromRaw16(uint16_t raw) {
            return ColorRGB::RGB(((raw >> 11) & 0x1f) << 3, ((raw >> 5) & 0x3f) << 2, (raw & 0x1f) << 3);
        }

        // shorthand method with implicit bpp based on type deduction for easier use in templates
        static constexpr ColorRGB fromRaw(uint16_t raw) { return fromRaw16(raw); }

        constexpr uint8_t r() const { return (raw_ >> 16) & 0xff; }
        constexpr uint8_t g() const { return (raw_ >> 8) & 0xff; }
        constexpr uint8_t b() const { return raw_ & 0xff; }

        constexpr void setR(uint8_t v) { raw_ = (raw_ & 0xff00ffff) | (v << 16); }
        constexpr void setG(uint8_t v) { raw_ = (raw_ & 0xffff0000) | (v << 8); }
        constexpr void setB(uint8_t v) { raw_ = (raw_ & 0xffffff00) | v; }

        /** Returns the RGB color converted to 565 16bit raw value.
         */
        constexpr uint16_t raw16() const {
            return ((r() >> 3) << 11) | ((g() >> 2) << 5) | (b() >> 3);
        }

        /** Returns the RGB color converted to 332 8bit raw value.
         */
        constexpr uint8_t raw8() const {
            return ((r() >> 5) << 5) | ((g() >> 5) << 2) | (b() >> 6);
        }

        constexpr operator uint16_t() const {
            return raw16();
        }

        constexpr operator uint8_t() const {
            return raw8();
        }

        constexpr ColorRGB withAlpha(uint8_t a) const {
            return ColorRGB{r() * a / 255, g() * a / 255, b() * a / 255};
        }

        /** Takes base color and converts it to the 4 color array useful for 2bpp font rendering using black and four different levels of brightness.
         */
        constexpr std::array<uint16_t, 4> toFontColors() const {
            return {
                0, 
                withAlpha(85), 
                withAlpha(170),
                raw16(),
            };
        }

        static constexpr ColorRGB White() { return ColorRGB{255, 255, 255}; }

    private:
        uint32_t raw_ = 0;
    }); // rckid::ColorRGB

    static_assert(sizeof(ColorRGB) == 4);

    /** Indexed color. 
     
        Represents and index to a palette of 256 colors with 8 bpp.
     */
    PACKED(class Color256 {
    public:
        static constexpr uint8_t BPP = 8;
        static constexpr bool PALETTE = true;

        constexpr Color256() = default;
        static constexpr Color256 fromRaw(uint8_t raw) { return Color256{raw}; }
        constexpr uint16_t raw() const { return index_; }


        constexpr Color256(uint8_t index): index_{index} {}

        bool operator == (Color256 const & other) const { return index_ == other.index_; }
        bool operator != (Color256 const & other) const { return index_ != other.index_; }

    private:
        uint8_t index_ = 0;
    }); // rckid::Color256

    static_assert(sizeof(Color256) == 1);

} // namespace rckid
