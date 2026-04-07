#pragma once

#include <platform.h>
#include <platform/utils.h>

#include <rckid/error.h>
#include <rckid/serialization.h>
#include <rckid/graphics/geometry.h>

namespace rckid {

    /** RCKid Color space
     
        Colors in RCKid are represented using the Color class, which corresponds to a 24bit RGB color. However, due to hardware limitations, this is neither the color RCKid can display, nor the color that users are most likely to utilize due to its relatively large memory size. 

        The color class thus provides general color manipulations for which the 24bit RGB format is well suited, but also provides subclasses corresponding to the actual color representations used by the RCKid. Those include the native RGB565 format used by the display as well as more memory efficient indexed and lower RGB formats. Those color representations then provide methods for reading and writing puxel data from pixel buffers as well as pixel buffer creation. 

        For convenience, Color class itself defines their representation aware variants, which return and take uint16_t as pixel values as this is the largest color representation used.
     */
    class Color {
    public:

        enum class Representation {
            RGB565,
            RGB332,
            Index256,
            Index16,
        }; 

        class alignas(2) RGB565 {
        public:
            constexpr RGB565() = default;
            constexpr RGB565(uint16_t raw): raw_{raw} {}


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

            /** Returns the value of the pixel at (x, y) in the specified raw buffer. 
             
                Assumes native buffer orientation
             */
            static RGB565 getPixel(uint8_t const * buffer, Coord w, Coord h, Coord x, Coord y) {
                return RGB565{reinterpret_cast<uint16_t const *>(buffer)[mapIndexColumnFirst(x, y, w, h)]};
            }

            static void setPixel(uint8_t * buffer, Coord w, Coord h, Coord x, Coord y, RGB565 color) {
                reinterpret_cast<uint16_t *>(buffer)[mapIndexColumnFirst(x, y, w, h)] = static_cast<uint16_t>(color);
            }

            static constexpr uint32_t getPixelArraySize(Coord width, Coord height) {
                return width * height * sizeof(uint16_t);
            }

        private:
            uint16_t raw_ = 0;

        } __attribute__((packed)); // Color::RGB565

        class RGB332 {
        public:

            constexpr RGB332() = default;
            constexpr RGB332(uint16_t raw): 
                raw_{static_cast<uint8_t>(raw)}
            {
                ASSERT(raw < 256);
            }

            constexpr uint8_t r() const { return ((raw_ >> 5) & 0x7) * 255 / 7; }
            constexpr uint8_t g() const { return ((raw_ >> 2) & 0x7) * 255 / 7; }
            constexpr uint8_t b() const { return (raw_ & 0x3) * 255 / 3; }

            constexpr operator uint8_t() const { return raw_; }

            constexpr operator uint16_t() const {
                uint8_t r5 = (r() >> 3) & 0x1f;
                uint8_t g6 = (g() >> 2) & 0x3f;
                uint8_t b5 = (b() >> 3) & 0x1f;
                return static_cast<uint16_t>((r5 << 11) | (g6 << 5) | b5);
            }

            static RGB332 getPixel(uint8_t const * buffer, Coord w, Coord h, Coord x, Coord y) {
                return RGB332{buffer[mapIndexColumnFirst(x, y, w, h)]};
            }

            static void setPixel(uint8_t * buffer, Coord w, Coord h, Coord x, Coord y, RGB332 color) {
                buffer[mapIndexColumnFirst(x, y, w, h)] = static_cast<uint8_t>(color);
            }

            static constexpr uint32_t getPixelArraySize(Coord width, Coord height) {
                return width * height * sizeof(uint8_t);
            }

        private:
            uint8_t raw_ = 0;
        } __attribute__((packed)); // Color::RGB332

        class Index256 {
        public:
            static constexpr bool Indexed = true;
            static constexpr uint32_t BPP = 8;

            constexpr Index256() = default;
            explicit constexpr Index256(uint16_t raw): 
                raw_{static_cast<uint8_t>(raw)} 
            {
                ASSERT(raw < 256);
            }

            uint8_t index() const { return raw_; }

            constexpr operator uint8_t() const { return raw_; }

            static Index256 getPixel(uint8_t const * buffer, Coord w, Coord h, Coord x, Coord y) {
                return Index256{buffer[mapIndexColumnFirst(x, y, w, h)]};
            }

            static void setPixel(uint8_t * buffer, Coord w, Coord h, Coord x, Coord y, Index256 color) {
                buffer[mapIndexColumnFirst(x, y, w, h)] = static_cast<uint8_t>(color);
            }

            static constexpr uint32_t getPixelArraySize(Coord width, Coord height) {
                return width * height * sizeof(uint8_t);
            }

        private:
            uint8_t raw_ = 0;

        } __attribute__((packed));
        
        class Index16 {
        public:
            static constexpr bool Indexed = true;
            static constexpr uint32_t BPP = 4;

            constexpr Index16() = default;
            explicit constexpr Index16(uint16_t raw): 
                raw_{static_cast<uint8_t>(raw)}
            {
                ASSERT(raw < 16);
            }

            uint8_t index() const { return raw_; }

            constexpr operator uint8_t() const { return raw_; }

            static Index16 getPixel(uint8_t const * buffer, Coord w, Coord h, Coord x, Coord y) {
                uint32_t offset = mapIndexColumnFirst(x, y, w, h);
                uint8_t byte = buffer[offset / 2];
                byte = byte >> ((offset & 1) * 4);
                return Index16{static_cast<uint8_t>(byte & 0xf)};
            }

            static void setPixel(uint8_t * buffer, Coord w, Coord h, Coord x, Coord y, Index16 color) {
                uint32_t offset = mapIndexColumnFirst(x, y, w, h);
                uint8_t & byte = buffer[offset / 2];
                byte = byte & ((offset & 1) ? 0x0f : 0xf0);
                byte |= static_cast<uint8_t>(color) << ((offset & 1) * 4);
            }

            static constexpr uint32_t getPixelArraySize(Coord width, Coord height) {
                return (width * height) * sizeof(uint8_t) / 2;
            }

        private:
            uint8_t raw_ = 0;
        };

        static_assert(sizeof(RGB565) == 2, "Color::RGB565 must be exactly 2 bytes");

        constexpr Color(): r{0}, g{0}, b{0} {};

        constexpr Color(RGB565 value) : r{value.r()}, g{value.g()}, b{value.b()} {}

        constexpr Color(RGB332 value):  r{value.r()}, g{value.g()}, b{value.b()} {}

        static constexpr Color RGB(uint8_t r, uint8_t g, uint8_t b) {
            return Color{r, g, b};
        }

        /** Creates color based on the HSV model coordinates. 
         
            The code is straight from Adafruit Neopixel library.
        */
        static constexpr Color HSV(uint16_t h, uint8_t s, uint8_t v) {
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
            return RGB(red, green, blue);
        }

        constexpr Color withBrightness(uint8_t a) {
            return Color{static_cast<uint8_t>(r * a / 255), static_cast<uint8_t>(g * a / 255), static_cast<uint8_t>(b * a / 255)};
        }

        // TODO remove and use the implicit conversion instead
        constexpr RGB565 toRGB565() const {
            return RGB565{static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))};
        }

        constexpr RGB332 toRGB332() const {
            return RGB332{static_cast<uint8_t>(((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6))};
        }

        constexpr operator RGB565() const {
            return RGB565{static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))};
        }

        uint8_t r;
        uint8_t g;
        uint8_t b;

        static constexpr Color Black()    { return Color{  0,  0,  0}; }
        static constexpr Color White()    { return Color{255,255,255}; }
        static constexpr Color Red()      { return Color{255,  0,  0}; }
        static constexpr Color Green()    { return Color{  0,255,  0}; }
        static constexpr Color Blue()     { return Color{  0,  0,255}; }
        static constexpr Color Yellow()   { return Color{255,255,  0}; }
        static constexpr Color Cyan()     { return Color{  0,255,255}; }
        static constexpr Color Magenta()  { return Color{255,  0,255}; }
        static constexpr Color Gray()     { return Color{128,128,128}; }
        static constexpr Color DarkGray() { return Color{ 64, 64, 64}; }

        static uint16_t getPixel(Representation r, uint8_t const * buffer, Coord w, Coord h, Coord x, Coord y) {
            switch (r) {
                case Representation::RGB565:
                    return RGB565::getPixel(buffer, w, h, x, y);
                case Representation::RGB332:
                    return RGB332::getPixel(buffer, w, h, x, y);
                case Representation::Index256:
                    return Index256::getPixel(buffer, w, h, x, y);
                case Representation::Index16:
                    return Index16::getPixel(buffer, w, h, x, y);
            }
            UNREACHABLE;
        }

        static void setPixel(Representation r, uint8_t * buffer, Coord w, Coord h, Coord x, Coord y, uint16_t color) {
            switch (r) {
                case Representation::RGB565:
                    return RGB565::setPixel(buffer, w, h, x, y, RGB565{color});
                case Representation::RGB332:
                    return RGB332::setPixel(buffer, w, h, x, y, RGB332{color});
                case Representation::Index256:
                    return Index256::setPixel(buffer, w, h, x, y, Index256{color});
                case Representation::Index16:
                    return Index16::setPixel(buffer, w, h, x, y, Index16{color});
            }
            UNREACHABLE;
        }

        static uint32_t getPixelArraySize(Representation r, Coord width, Coord height) {
            switch (r) {
                case Representation::RGB565:
                    return RGB565::getPixelArraySize(width, height);
                case Representation::RGB332:
                    return RGB332::getPixelArraySize(width, height);
                case Representation::Index256:
                    return Index256::getPixelArraySize(width, height);
                case Representation::Index16:
                    return Index16::getPixelArraySize(width, height);
            }
            UNREACHABLE;
        }

        static bool requiresPalette(Representation r) {
            switch (r) {
                case Representation::RGB565:
                case Representation::RGB332:
                    return false;
                case Representation::Index256:
                case Representation::Index16:
                    return true;
            }
            UNREACHABLE;
        }

    private:

        constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_): r{r_}, g{g_}, b{b_} {}

    }; // rckid::Color


    inline uint32_t colorRepresentationBpp(Color::Representation rep) {
        switch (rep) {
            case Color::Representation::RGB565:
                return 16;
            case Color::Representation::RGB332:
                return 8;
            case Color::Representation::Index256:
                return 8;
            case Color::Representation::Index16:
                return 4;
            default:
                UNREACHABLE;
        }
    }

    inline void write(Writer & w, Color color) {
        w << "#" << hex2(color.r) << hex2(color.g) << hex2(color.b);
    }

    inline void read(Reader & reader, Color & color) {
        color = Color::Black(); // reset color in case of error
        if (reader.peekChar() != '#')
            return;
        reader.getChar(); // consume '#'
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        auto readHexByte = [&reader](uint8_t & into) -> bool {
            if (! platform::isHexDigit(reader.peekChar()))
                return false;
            into = platform::fromHex(reader.getChar()) << 4;
            if (! platform::isHexDigit(reader.peekChar()))
                return false;
            into |= platform::fromHex(reader.getChar());
            return true;
        };
        if (! readHexByte(r) || ! readHexByte(g) || ! readHexByte(b))
            return;
        color = Color::RGB(r, g, b);
    }

} // namespace rckid