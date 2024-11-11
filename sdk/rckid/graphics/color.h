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

        constexpr ColorRGB(int r, int g, int b): raw_{0} {
            setR(static_cast<uint8_t>(r & 0xff));
            setG(static_cast<uint8_t>(g & 0xff));
            setB(static_cast<uint8_t>(b & 0xff));
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
            raw_{static_cast<uint8_t>(color & 0xf)} {
        }

        constexpr Color16(Color16 const &) = default;
        constexpr Color16 & operator = (Color16 const &) = default;

        constexpr static Color16 fromRaw(uint8_t raw) { return Color16{raw}; }
        constexpr uint8_t toRaw() const { return raw_; }

        constexpr bool operator == (Color16 const & other) { return raw_ == other.raw_; }
        constexpr bool operator != (Color16 const & other) { return raw_ != other.raw_; }

        constexpr uint8_t operator + (uint8_t index) const { return index + raw_; }

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