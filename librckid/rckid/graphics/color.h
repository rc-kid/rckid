#pragma once

#include <cstdint>

namespace rckid {

    /** A 16bit color in 565 bits per color format.
     
        Supports 65536 colors in a simple format native to the ST7789 screen used. 
     */
    class Color {
    public:
        constexpr Color():raw_{0} {}

        constexpr Color(uint8_t r, uint8_t g, uint8_t b): raw_{0} {
            setR(r);
            setG(g);
            setB(b);
        }

        static constexpr Color RGB(uint8_t r, uint8_t g, uint8_t b) { return Color{r, g, b}; }

        static constexpr Color Raw565(uint16_t rgb565) { return Color{rgb565}; }

        uint8_t r() const { return ((raw_ >> 11) & 0xff) << 3; }
        uint8_t g() const { return ((raw_ >> 5) & 0x3f) << 2; }
        uint8_t b() const { return (raw_ & 0xff) << 3; }

        constexpr void setR(uint8_t v) { uint16_t x = v >>= 3; raw_ = (raw_ & 0x07ff) | (x << 11); }
        constexpr void setG(uint8_t v) { uint16_t x = v >>= 2; raw_ = (raw_ & 0xf81f) | (x << 5); }
        constexpr void setB(uint8_t v) { raw_ = (raw_ & 0xffe0) | (v >> 3); }

        uint16_t rawValue16() const { return raw_; }

        static constexpr Color White() { return RGB(255, 255, 255); }
        static constexpr Color Blue() { return RGB(0, 0, 255); }

    private:

        constexpr Color(uint16_t raw): raw_{raw} {}

        uint16_t raw_;

    } __attribute__((packed)); // rckid::ColorRGB

    static_assert(sizeof(Color) == 2);

}; // namespace rckid