#pragma once

#include <cstdint>
#include "palette_332.h"

namespace rckid {

    class ColorRGB;
    class ColorRGB_332;

    /** Full RGB 565 color. 
     
        This is the native 5-6-5 bits per color channel (R, G, B) resolution of the display and the highest one that RCKid supports (the display also supports 6-6-6 3byte encoding which RCKid does not support). As a native color resolution, this is an equivalent of a true color resolution in the sense that each pixel contains its own color independent of any others. 
     */
    class ColorRGB {
    public:
        using RawType = uint16_t;
        static constexpr size_t BPP = 16;

        constexpr ColorRGB() = default;

        constexpr ColorRGB(uint8_t r, uint8_t g, uint8_t b): raw_{0} {
            setR(r);
            setG(g);
            setB(b);
        }

        constexpr ColorRGB(ColorRGB const & other): raw_{other.raw_} {}
        constexpr ColorRGB(ColorRGB_332 const & other); 

        static constexpr ColorRGB RGB(uint8_t r, uint8_t g, uint8_t b) { return ColorRGB{r, g, b}; }

        static constexpr ColorRGB Raw565(uint16_t rgb565) { return ColorRGB{rgb565}; }

        ColorRGB toRGB() const __attribute__((always_inline)) { return *this; }

        uint8_t r() const { return ((raw_ >> 11) & 0xff) << 3; }
        uint8_t g() const { return ((raw_ >> 5) & 0x3f) << 2; }
        uint8_t b() const { return (raw_ & 0xff) << 3; }

        constexpr void setR(uint8_t v) { uint16_t x = v >>= 3; raw_ = (raw_ & 0x07ff) | (x << 11); }
        constexpr void setG(uint8_t v) { uint16_t x = v >>= 2; raw_ = (raw_ & 0xf81f) | (x << 5); }
        constexpr void setB(uint8_t v) { raw_ = (raw_ & 0xffe0) | (v >> 3); }

        uint16_t rawValue16() const { return raw_; }

        ColorRGB withAlpha(uint8_t a) {
            return ColorRGB{
                static_cast<uint8_t>(static_cast<unsigned>(r()) * a / 3 & 0xff),
                static_cast<uint8_t>(static_cast<unsigned>(g()) * a / 3 & 0xff),
                static_cast<uint8_t>(static_cast<unsigned>(b()) * a / 3 & 0xff) 
            };
        }

#define COLOR(NAME, RED, GREEN, BLUE) static constexpr ColorRGB NAME() { return RGB(RED, GREEN, BLUE); }
#include "colors.inc.h"

    private:

        explicit constexpr ColorRGB(uint16_t raw): raw_{raw} {}

        uint16_t raw_ = 0;

    } __attribute__((packed)); // rckid::ColorRGB

    static_assert(sizeof(ColorRGB) == 2);


    /** 332 Color Space

        3bit values: 0 36 73 109 146 182 218 255
        2bit values: 0 85 170 255    
     */
    class ColorRGB_332 {
    public:
        using RawType = uint8_t;
        static constexpr size_t BPP = 8;

        constexpr ColorRGB_332() = default;

        constexpr ColorRGB_332(uint8_t r, uint8_t g, uint8_t b): raw_{0} {
            setR(r);
            setG(g);
            setB(b); 
        }

        static constexpr ColorRGB_332 RGB(uint8_t r, uint8_t g, uint8_t b) {
            return ColorRGB_332{r, g, b};
        }

        ColorRGB toRGB() const __attribute__((always_inline)) {
            return ColorRGB{r(), g(), b()};
        }

        uint8_t r() const { return CH3[(raw_ >> 5) & 0x7]; }
        uint8_t g() const { return CH3[(raw_ >> 2) & 0x7]; }
        uint8_t b() const { return CH2[(raw_ & 0x3)]; }

        constexpr void setR(uint8_t r) {
            raw_ &= 0b00011111;
            raw_ |= (std::min(255, static_cast<int>(r) + 16) & 0b11100000);
        }

        constexpr void setG(uint8_t g) {
            raw_ &= 0b11100011;
            raw_ |= (std::min(255, static_cast<int>(g) + 16) >> 3) & 0b00011100;
        }

        constexpr void setB(uint8_t b) {
            raw_ &= 0b11111100;
            raw_ |= (std::min(255, static_cast<int>(b) + 32) >> 6) & 0b00000011;
        }

        uint8_t rawValue8() const { return raw_; }

        ColorRGB_332 withAlpha(uint8_t a) {
            return ColorRGB_332{
                static_cast<uint8_t>(static_cast<unsigned>(r()) * a / 3 & 0xff),
                static_cast<uint8_t>(static_cast<unsigned>(g()) * a / 3 & 0xff),
                static_cast<uint8_t>(static_cast<unsigned>(b()) * a / 3 & 0xff) 
            };
        }

#define COLOR(NAME, RED, GREEN, BLUE) static constexpr ColorRGB_332 NAME() { return RGB(RED, GREEN, BLUE); }
#include "colors.inc.h"

    private:

        ColorRGB_332(uint8_t raw): raw_{raw} {}

        uint8_t raw_ = 0;

        static constexpr uint8_t CH3[] = {0, 36, 73, 109, 146, 182, 218, 255};
        static constexpr uint8_t CH2[] = {0, 85, 170, 255};


    } __attribute__((packed)); // ColorRGB_332

    static_assert(sizeof(ColorRGB_332) == 1);

    inline constexpr ColorRGB::ColorRGB(ColorRGB_332 const & other) {
        raw_ = other.toRGB().raw_;
    }

}; // namespace rckid


