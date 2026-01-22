#pragma once

#include <platform.h>

namespace rckid {

    /** Defines the colorspace.
     */
    class Color {
    public:
        class RGB565 {
        public:
            constexpr RGB565() = default;
            explicit constexpr RGB565(uint16_t raw): raw_{raw} {}


            constexpr uint8_t r() const {
                uint8_t result = (raw_ >> 11) & 0x1f;
                return (result << 3) | (result >> 2);
            }

            constexpr uint8_t g() const {
                uint8_t result = (raw_ >> 5) & 0x3f;
                return (result << 2) | (result >> 4);
            }

            constexpr uint8_t b() const {
                uint8_t result = raw_ & 0x1f;
                return (result << 3) | (result >> 2);
            }

            constexpr operator uint16_t() const { return raw_; }

        private:
            uint16_t raw_ = 0;

        } __attribute__((packed, aligned(2))); // Color::RGB565

        static_assert(sizeof(RGB565) == 2, "Color::RGB565 must be exactly 2 bytes");

        constexpr Color(): r{0}, g{0}, b{0} {};

        constexpr Color(RGB565 value) : r{value.r()}, g{value.g()}, b{value.b()} {}

        static constexpr Color RGB(uint8_t r, uint8_t g, uint8_t b) {
            return Color{r, g, b};
        }

        constexpr RGB565 toRGB565() const {
            return RGB565{static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))};
        }

        uint8_t r;
        uint8_t g;
        uint8_t b;

        static constexpr Color Black()   { return Color{0,0,0}; }
        static constexpr Color White()   { return Color{255,255,255}; }
        static constexpr Color Red()     { return Color{255,0,0}; }
        static constexpr Color Green()   { return Color{0,255,0}; }
        static constexpr Color Blue()    { return Color{0,0,255}; }
        static constexpr Color Yellow()  { return Color{255,255,0}; }
        static constexpr Color Cyan()    { return Color{0,255,255}; }
        static constexpr Color Magenta() { return Color{255,0,255}; }
        static constexpr Color Gray()    { return Color{128,128,128}; }

    private:

        constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_): r{r_}, g{g_}, b{b_} {}

    }; // rckid::Color

} // namespace rckid