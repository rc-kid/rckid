#pragma once

#include <cstdint>

namespace rckid {

    /** Display pixel. 

        The 16 bits are used as RRRRRGGG GGGBBBBB. 
        111
     */
    class Color {
    public:

        constexpr Color():raw_{0} {}

        uint8_t r() const { return ((raw_ >> 11) & 0xff) << 3; }
        uint8_t g() const { return ((raw_ >> 5) & 0x3f) << 2; }
        uint8_t b() const { return (raw_ & 0xff) << 3; }

        constexpr void setR(uint8_t v) { uint16_t x = v >>= 3; raw_ = (raw_ & 0x07ff) | (x << 11); }
        constexpr void setG(uint8_t v) { uint16_t x = v >>= 2; raw_ = (raw_ & 0xf81f) | (x << 5); }
        constexpr void setB(uint8_t v) { raw_ = (raw_ & 0xffe0) | (v >> 3); }

        uint16_t rawValue16() const { return raw_; }

        static constexpr Color Black() { return Color{0}; }
        static constexpr Color White() { return Color{0xffff}; }
        static constexpr Color Red() { return Color{0xf800}; }
        static constexpr Color Green() { return Color{0x7e0}; }
        static constexpr Color Blue() { return Color{0x001f}; }

        static constexpr Color RGB(uint8_t r, uint8_t g, uint8_t b) {
            Color c;
            c.setR(r);
            c.setG(g);
            c.setB(b);
            return c;
        }

    private:

        constexpr Color(uint16_t raw): raw_{raw} {}

        uint16_t raw_;
        
    } __attribute__((packed)); 

    static_assert(sizeof(Color) == 2);



    class Rect {
    public:

        unsigned left() const { return left_; }
        unsigned right() const { return left_ + width_; }
        unsigned top() const { return top_; }
        unsigned bottom() const { return top_ + height_; }
        unsigned width() const { return width_; }
        unsigned height() const { return height_; }

        static constexpr Rect WH(unsigned width, unsigned height) { return Rect{0, 0, (uint16_t)width, (uint16_t)height}; }

    private:

        constexpr Rect(uint16_t l, uint16_t t, uint16_t w, uint16_t h):left_{l}, top_{t}, width_{w}, height_{h} {}

        uint16_t left_;
        uint16_t top_;
        uint16_t width_;
        uint16_t height_;
    }; // Rect

} // namespace rckid