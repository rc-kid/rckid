#pragma once

#include <array>
#include <platform.h>
#include <platform/utils.h>
#include "../utils/string.h"

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

        /** Creates color based on the HSV model coordinates. 
         
            The code is straight from Adafruit Neopixel library.
        */
        static constexpr ColorRGB HSV(uint16_t h, uint8_t s, uint8_t v) {
            uint8_t red = 0, green = 0, blue = 0;
            // Remap 0-65535 to 0-1529. Pure red is CENTERED on the 64K rollover;
            // 0 is not the start of pure red, but the midpoint...a few values above
            // zero and a few below 65536 all yield pure red (similarly, 32768 is the
            // midpoint, not start, of pure cyan). The 8-bit RGB hexcone (256 values
            // each for red, green, blue) really only allows for 1530 distinct hues
            // (not 1536, more on that below), but the full unsigned 16-bit type was
            // chosen for hue so that one's code can easily handle a contiguous color
            // wheel by allowing hue to roll over in either direction.
            h = (h * 1530L + 32768) / 65536;
            // Because red is centered on the rollover point (the +32768 above,
            // essentially a fixed-point +0.5), the above actually yields 0 to 1530,
            // where 0 and 1530 would yield the same thing. Rather than apply a
            // costly modulo operator, 1530 is handled as a special case below.

            // So you'd think that the color "hexcone" (the thing that ramps from
            // pure red, to pure yellow, to pure green and so forth back to red,
            // yielding six slices), and with each color component having 256
            // possible values (0-255), might have 1536 possible items (6*256),
            // but in reality there's 1530. This is because the last element in
            // each 256-element slice is equal to the first element of the next
            // slice, and keeping those in there this would create small
            // discontinuities in the color wheel. So the last element of each
            // slice is dropped...we regard only elements 0-254, with item 255
            // being picked up as element 0 of the next slice. Like this:
            // Red to not-quite-pure-yellow is:        255,   0, 0 to 255, 254,   0
            // Pure yellow to not-quite-pure-green is: 255, 255, 0 to   1, 255,   0
            // Pure green to not-quite-pure-cyan is:     0, 255, 0 to   0, 255, 254
            // and so forth. Hence, 1530 distinct hues (0 to 1529), and hence why
            // the constants below are not the multiples of 256 you might expect.

            // Convert hue to R,G,B (nested ifs faster than divide+mod+switch):
            if (h < 510) {         // Red to Green-1
                blue = 0;
                if(h < 255) {       //   Red to Yellow-1
                    red = 255;
                    green = h;            //     g = 0 to 254
                } else {              //   Yellow to Green-1
                    red = 510 - h;      //     r = 255 to 1
                    green = 255;
                }
            } else if (h < 1020) { // Green to Blue-1
                red = 0;
                if (h <  765) {      //   Green to Cyan-1
                    green = 255;
                    blue = h - 510;      //     b = 0 to 254
                } else {              //   Cyan to Blue-1
                    green = 1020 - h;     //     g = 255 to 1
                    blue = 255;
                }
            } else if(h < 1530) { // Blue to Red-1
                green = 0;
                if (h < 1275) {      //   Blue to Magenta-1
                    red = h - 1020;     //     r = 0 to 254
                    blue = 255;
                } else {              //   Magenta to Red-1
                    red = 255;
                    blue = 1530 - h;     //     b = 255 to 1
                }
            } else {                // Last 0.5 Red (quicker than % operator)
                red = 255;
                green = blue = 0;
            }

            // Apply saturation and value to R,G,B, pack into 32-bit result:
            uint32_t v1 =   1 + v; // 1 to 256; allows >>8 instead of /255
            uint16_t s1 =   1 + s; // 1 to 256; same reason
            uint8_t  s2 = 255 - s; // 255 to 0
            red = ((((red * s1) >> 8) + s2) * v1) >> 8;
            green = ((((green * s1) >> 8) + s2) * v1) >> 8;
            blue = ((((blue * s1) >> 8) + s2) * v1) >> 8;
            return ColorRGB{red, green, blue};
        }

        static constexpr ColorRGB fromRaw16(uint16_t raw) {
            return ColorRGB::RGB(((raw >> 11) & 0x1f) << 3, ((raw >> 5) & 0x3f) << 2, (raw & 0x1f) << 3);
        }

        // shorthand method with implicit bpp based on type deduction for easier use in templates
        static constexpr ColorRGB fromRaw(uint32_t raw) { 
            ASSERT(raw <= 0xffff);
            return ColorRGB::RGB(((raw >> 11) & 0x1f) << 3, ((raw >> 5) & 0x3f) << 2, (raw & 0x1f) << 3);
        }

        static constexpr ColorRGB fromString(char const * str) {
            ASSERT(str[0] == '#');
            uint32_t r = platform::fromHex(str[1]) << 4 | platform::fromHex(str[2]);
            uint32_t g = platform::fromHex(str[3]) << 4 | platform::fromHex(str[4]);
            uint32_t b = platform::fromHex(str[5]) << 4 | platform::fromHex(str[6]);
            return ColorRGB::RGB(r, g, b);
        }

        String toString() const {
            using namespace platform;
            return STR('#' << fillLeft(hex(r(), false), 2) << fillLeft(hex(g(), false), 2) << fillLeft(hex(b(), false), 2));
        }

        constexpr uint32_t toRaw() const {
            return ((r() >> 3) << 11) | ((g() >> 2) << 5) | (b() >> 3);
        }

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

        static constexpr ColorRGB White()     { return ColorRGB{255, 255, 255}; }
        static constexpr ColorRGB Black()     { return ColorRGB{0,   0,   0  }; }
        static constexpr ColorRGB Red()       { return ColorRGB{255, 0,   0  }; }
        static constexpr ColorRGB Green()     { return ColorRGB{0,   255, 0  }; }
        static constexpr ColorRGB Blue()      { return ColorRGB{0,   0,   255}; }
        static constexpr ColorRGB Gray()      { return ColorRGB{192, 192, 192}; }
        static constexpr ColorRGB DarkGray()  { return ColorRGB{64,  64,  64 }; }
        static constexpr ColorRGB DarkRed()   { return ColorRGB{128, 0,   0  }; }
        static constexpr ColorRGB DarkGreen() { return ColorRGB{0,   128, 0  }; }
        static constexpr ColorRGB DarkBlue()  { return ColorRGB{0,   0,   128}; }

        static constexpr ColorRGB Yellow()    { return ColorRGB{255, 255, 0  }; }
        static constexpr ColorRGB Violet()    { return ColorRGB{255, 0,   255}; }
        static constexpr ColorRGB Cyan()      { return ColorRGB{0,   255, 255}; }

        constexpr bool operator == (ColorRGB const & other) const { return raw_ == other.raw_; }
        constexpr bool operator != (ColorRGB const & other) const { return raw_ != other.raw_; }
        
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
        static constexpr Color256 fromRaw(uint32_t raw) { 
            ASSERT(raw <= 0xff);
            return Color256{static_cast<uint8_t>(raw & 0xff)}; 
        }

        constexpr uint32_t toRaw() const { return index_; }

        constexpr Color256(uint8_t index): index_{index} {}

        bool operator == (Color256 const & other) const { return index_ == other.index_; }
        bool operator != (Color256 const & other) const { return index_ != other.index_; }

    private:
        uint8_t index_ = 0;
    }); // rckid::Color256

    static_assert(sizeof(Color256) == 1);

    PACKED(class Color16 {
    public:
        static constexpr uint8_t BPP = 4;
        static constexpr bool PALETTE = true;

        constexpr Color16() = default;
        static constexpr Color16 fromRaw(uint32_t raw) { 
            ASSERT(raw <= 0xf);
            return Color16{static_cast<uint8_t>(raw & 0xf)}; 
        }

        constexpr uint32_t toRaw() const { return index_; }

        constexpr Color16(uint8_t index): index_{index} {}

        bool operator == (Color16 const & other) const { return index_ == other.index_; }
        bool operator != (Color16 const & other) const { return index_ != other.index_; }

    private:
        uint8_t index_ = 0;
    });

} // namespace rckid
