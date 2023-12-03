#pragma once

#include <cstdint>

namespace rckid {

    /** A 16bit color in 565 bits per color format.
     
        Supports 65536 colors in a simple format native to the ST7789 screen used. 
     */
    class ColorRGB {
    public:
        constexpr ColorRGB():raw_{0} {}

        constexpr ColorRGB(uint8_t r, uint8_t g, uint8_t b): raw_{0} {
            setR(r);
            setG(g);
            setB(b);
        }

        static constexpr ColorRGB RGB(uint8_t r, uint8_t g, uint8_t b) { return ColorRGB{r, g, b}; }

        uint8_t r() const { return ((raw_ >> 11) & 0xff) << 3; }
        uint8_t g() const { return ((raw_ >> 5) & 0x3f) << 2; }
        uint8_t b() const { return (raw_ & 0xff) << 3; }

        constexpr void setR(uint8_t v) { uint16_t x = v >>= 3; raw_ = (raw_ & 0x07ff) | (x << 11); }
        constexpr void setG(uint8_t v) { uint16_t x = v >>= 2; raw_ = (raw_ & 0xf81f) | (x << 5); }
        constexpr void setB(uint8_t v) { raw_ = (raw_ & 0xffe0) | (v >> 3); }

        uint16_t rawValue16() const { return raw_; }

    private:
        uint16_t raw_;

    } __attribute__((packed)); // rckid::ColorRGB

    static_assert(sizeof(ColorRGB) == 2);


    /** A 16bit color with 4 bits per channel + 4bits alpha. 
     
        Since the alpha channel is discraded when sending the color to the display, supports 4096 colors only. Inspired, and binary compatible with the Pimoroni's Picosystem SDK. 
     */
    class ColorRGBA {
    public:
        constexpr ColorRGBA():raw_{0} {}

        constexpr ColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255): raw_{0} {
            setR(r);
            setG(g);
            setB(b);
            setA(0);
        }

        static constexpr ColorRGBA RGB(uint8_t r, uint8_t g, uint8_t b) { return ColorRGBA{r, g, b, 255}; }

        uint8_t r() const { return ((raw_ >> 12) & 0xf) << 4; }
        uint8_t g() const { return ((raw_ >> 8) & 0xf) << 4; }
        uint8_t b() const { return ((raw_ >> 4) & 0xf) << 4; }
        uint8_t a() const { return ((raw_ & 0xf)); }

        constexpr void setR(uint8_t v) { raw_ = (raw_ & 0x0fff) | ((uint16_t)(v >> 4) << 12); }
        constexpr void setG(uint8_t v) { raw_ = (raw_ & 0xf0ff) | ((uint16_t)(v >> 4) << 8); }
        constexpr void setB(uint8_t v) { raw_ = (raw_ & 0xff0f) | ((uint16_t)(v >> 4) << 4); }
        constexpr void setA(uint8_t v) { raw_ = (raw_ & 0xfff0) | (v >> 4); }
            
        uint16_t rawValue16() const { return raw_; }

    private:
        uint16_t raw_ = 0;
    } __attribute__((packed)); // rckid::ColorRGBA

    static_assert(sizeof(ColorRGBA) == 2);



}; // namespace rckid