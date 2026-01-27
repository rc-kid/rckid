#pragma once

#include <platform.h>

#include <rckid/error.h>
#include <rckid/graphics/geometry.h>

namespace rckid {

    /** RCKid Color space
     
        Colors in RCKid are represented using the Color class, which corresponds to a 24bit RGB color. However, due to hardware limitations, this is neither the color RCKid can display, nor the color that users are most likely to utilize due to its relatively large memory size. 

        THe color class thus provides general color manipulations for which the 24bit RGB format is well suited, butt also provides subclasses corresponding to the actual color representations used by the RCKid. Those include the native RGB565 format used by the display as well as more memory efficient indexed and lower RGB formats. Those color representations then provide methods for reading and writing puxel data from pixel buffers as well as pixel buffer creation. 

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

            /** Returns the value of the pixel at (x, y) in the specified raw buffer. 
             
                Assumes native buffer orientation
             */
            static RGB565 getPixel(uint8_t const * buffer, Coord w, Coord h, Coord x, Coord y) {
                return RGB565{reinterpret_cast<uint16_t const *>(buffer)[mapIndexColumnFirst(x, y, w, h)]};
            }

            static void setPixel(uint8_t * buffer, Coord w, Coord h, Coord x, Coord y, RGB565 color) {
                reinterpret_cast<uint16_t *>(buffer)[mapIndexColumnFirst(x, y, w, h)] = static_cast<uint16_t>(color);
            }

            static uint32_t getPixelArraySize(Coord width, Coord height) {
                return width * height * sizeof(uint16_t);
            }

        private:
            uint16_t raw_ = 0;

        } __attribute__((packed, aligned(2))); // Color::RGB565

        class RGB332 {
        public:

            constexpr RGB332() = default;
            explicit constexpr RGB332(uint16_t raw): 
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

            static uint32_t getPixelArraySize(Coord width, Coord height) {
                return width * height * sizeof(uint8_t);
            }

        private:
            uint8_t raw_ = 0;
        } __attribute__((packed)); // Color::RGB332

        class Index256 {
        public:
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

            static uint32_t getPixelArraySize(Coord width, Coord height) {
                return width * height * sizeof(uint8_t);
            }

        private:
            uint8_t raw_ = 0;

        } __attribute__((packed));
        
        class Index16 {
        public:
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
                return Index16{byte & 0xf};
            }

            static void setPixel(uint8_t * buffer, Coord w, Coord h, Coord x, Coord y, Index16 color) {
                uint32_t offset = mapIndexColumnFirst(x, y, w, h);
                uint8_t & byte = buffer[offset / 2];
                byte = byte & ((offset & 1) ? 0x0f : 0xf0);
                byte |= static_cast<uint8_t>(color) << ((offset & 1) * 4);
            }

            static uint32_t getPixelArraySize(Coord width, Coord height) {
                return (width * height) * sizeof(uint8_t) / 2;
            }

        private:
            uint8_t raw_ = 0;
        };

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

} // namespace rckid