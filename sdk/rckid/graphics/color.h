#pragma once

#include <platform.h>

// on windows, there is RGB macro, which clases with the 565 and 332 colors
#ifdef RGB
#undef RGB
#endif

namespace rckid {

    PACKED(class ColorRGB565 {
    public:

        static constexpr uint8_t BPP = 16;

        constexpr ColorRGB565() = default;

        static constexpr ColorRGB565 fromRaw(uint16_t raw) { return ColorRGB565{raw}; }
        constexpr uint16_t raw() const { return raw_; }

        constexpr ColorRGB565(int r, int g, int b): raw_{0} {
            setR(static_cast<uint8_t>(r & 0xff));
            setG(static_cast<uint8_t>(g & 0xff));
            setB(static_cast<uint8_t>(b & 0xff));
        }

        static constexpr ColorRGB565 RGB(int r, int g, int b) {
            return ColorRGB565(r, g, b);
        }

        constexpr uint8_t r() const { return ((raw_ >> 11) & 0xff) << 3; }
        constexpr uint8_t g() const { return ((raw_ >> 5) & 0x3f) << 2; }
        constexpr uint8_t b() const { return (raw_ & 0xff) << 3; }

        constexpr void setR(uint8_t v) { uint16_t x = v >>= 3; raw_ = (raw_ & 0x07ff) | (x << 11); }
        constexpr void setG(uint8_t v) { uint16_t x = v >>= 2; raw_ = (raw_ & 0xf81f) | (x << 5); }
        constexpr void setB(uint8_t v) { raw_ = (raw_ & 0xffe0) | (v >> 3); }

        constexpr ColorRGB565 withAlpha(uint8_t a) {
            return ColorRGB565{r() * a / 255, g() * a / 255, b() * a / 255};
        }

    private:
        constexpr ColorRGB565(uint16_t raw): raw_{raw} {}

        uint16_t raw_ = 0;

    }); // rckid::ColorRGB565

    static_assert(sizeof(ColorRGB565) == 2);

    PACKED(class ColorRGB332 {
    public:
        static constexpr uint8_t BPP = 8;

    }); // rckid::ColorRGB332

    static_assert(sizeof(ColorRGB332) == 1);

    PACKED(class Color256 {
    public:
        static constexpr uint8_t BPP = 8;
        constexpr Color256() = default;
        static constexpr Color256 fromRaw(uint8_t raw) { return Color256{raw}; }
        constexpr uint16_t raw() const { return index_; }


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
        constexpr Color16() = default;
        static constexpr Color16 fromRaw(uint8_t raw) { return Color16{static_cast<uint8_t>(raw & 0xf)}; }
        constexpr uint16_t raw() const { return index_; }

        constexpr Color16(uint8_t index): index_{index} {}

        bool operator == (Color16 const & other) const { return index_ == other.index_; }
        bool operator != (Color16 const & other) const { return index_ != other.index_; }


    private:
        uint8_t index_ = 0;

    }); // rckid::Color256

    static_assert(sizeof(Color16) == 1);

    using ColorRGB = ColorRGB565;

} // namespace rckid
