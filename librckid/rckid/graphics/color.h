#pragma once

#include <cstdint>

namespace rckid {

    /** Full RGB 565 color. 
     
        This is the native 5-6-5 bits per color channel (R, G, B) resolution of the display and the highest one that RCKid supports (the display also supports 6-6-6 3byte encoding which RCKid does not support). As a native color resolution, this is an equivalent of a true color resolution in the sense that each pixel contains its own color independent of any others. 
     */
    class ColorRGB {
    public:
        static constexpr size_t BPP = 16;

        constexpr ColorRGB():raw_{0} {}

        constexpr ColorRGB(uint8_t r, uint8_t g, uint8_t b): raw_{0} {
            setR(r);
            setG(g);
            setB(b);
        }

        static constexpr ColorRGB RGB(uint8_t r, uint8_t g, uint8_t b) { return ColorRGB{r, g, b}; }

        static constexpr ColorRGB Raw565(uint16_t rgb565) { return ColorRGB{rgb565}; }

        ColorRGB toRGB() const { return *this; }

        uint8_t r() const { return ((raw_ >> 11) & 0xff) << 3; }
        uint8_t g() const { return ((raw_ >> 5) & 0x3f) << 2; }
        uint8_t b() const { return (raw_ & 0xff) << 3; }

        constexpr void setR(uint8_t v) { uint16_t x = v >>= 3; raw_ = (raw_ & 0x07ff) | (x << 11); }
        constexpr void setG(uint8_t v) { uint16_t x = v >>= 2; raw_ = (raw_ & 0xf81f) | (x << 5); }
        constexpr void setB(uint8_t v) { raw_ = (raw_ & 0xffe0) | (v >> 3); }

        uint16_t rawValue16() const { return raw_; }

        static constexpr ColorRGB White() { return RGB(255, 255, 255); }
        static constexpr ColorRGB Black() { return RGB(0,0,0); }
        static constexpr ColorRGB Blue() { return RGB(0, 0, 255); }

    private:

        explicit constexpr ColorRGB(uint16_t raw): raw_{raw} {}

        uint16_t raw_;

    } __attribute__((packed)); // rckid::ColorRGB

    static_assert(sizeof(ColorRGB) == 2);

    /** Color from a 256 color palette represented by a single byte (index to the palette). 
     
        Uses the 6-8-5 levels per channel for a total of 240 color + 16 true grays. This should give the best representation of the whole spectrum in a single byte according to [1].

        The alternative would be to use a dedicated palette with editable colors, but this would greatly complicate the RGB constructor. 

        [1] https://en.wikipedia.org/wiki/List_of_software_palettes
     */
    class Color256 {
    public:
        static constexpr size_t BPP = 8;

        uint8_t index() const { return raw_; }

        static Color256 RGB(uint8_t r, uint8_t g, uint8_t b) {
            if (r == g && g == b)
                return 240 + (r >> 4);
            else
                return 40 * (r / 43) + 5 * (g / 32) + (b / 52);
        }

        static Color256 RGB(ColorRGB rgb) { return RGB(rgb.r(), rgb.g(), rgb.b()); }

        ColorRGB toRGB() const {
            if (raw_ >= 240) {
                uint8_t i = raw_ - 240;
                i = (i << 4) + i;
                return ColorRGB{i, i, i};
            } else {
                uint8_t b = raw_ % 5;
                uint8_t g = raw_ / 5;
                uint8_t r = g / 8;
                g = g % 8;
                // TODO this is not precise, revisit and fix
                return ColorRGB{static_cast<uint8_t>(r * 43), static_cast<uint8_t>(g * 32), static_cast<uint8_t>(b * 52)};
            }
        }

        static constexpr Color256 White() { return Color256{255}; }
        static constexpr Color256 Black() { return Color256{0}; }
        static constexpr Color256 Blue() { return Color256{4}; }

    private:

        constexpr Color256(uint8_t index): raw_{index} {}

        uint8_t raw_;
    } __attribute__((packed)); // rckid::Color256

    static_assert(sizeof(Color256) == 1);

}; // namespace rckid