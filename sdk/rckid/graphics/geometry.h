#pragma once

#include <cstdint>

namespace rckid {

    using Coord = int32_t;


    template<typename COORD>
    class TPoint {
    public:
        COORD x = COORD{0};
        COORD y = COORD{0};

        TPoint() = default;
        constexpr TPoint(COORD x, COORD y): x{x}, y{y} {}

        constexpr static TPoint origin() { return TPoint{0,0}; }

        constexpr TPoint operator + (TPoint other) const { return TPoint{x + other.x, y + other.y}; }
        constexpr TPoint operator - (TPoint other) const { return TPoint{x - other.x, y - other.y}; }

        constexpr TPoint operator * (COORD other) const { return TPoint{x * other, y * other}; }

        constexpr TPoint & operator += (TPoint other) {
            x += other.x;
            y += other.y;
            return *this;
        }
        
        constexpr bool operator == (TPoint other) const { return x == other.x && y == other.y; }

    }; // rckid::Point

    template<typename COORD>
    class TRect {
    public:
        COORD x = COORD{0};
        COORD y = COORD{0};
        COORD w = COORD{0};
        COORD h = COORD{0};

        constexpr COORD top() const { return y; }
        constexpr COORD left() const { return x; }
        constexpr COORD bottom() const { return y + h; }
        constexpr COORD right() const { return x + w; }

        constexpr TPoint<COORD> topLeft() const { return TPoint<COORD>{x, y}; }
        constexpr TPoint<COORD> bottomRight() const { return TPoint<COORD>{right(), bottom()}; }

        static constexpr TRect WH(int width, int height) {
            return TRect{0, 0, width, height};
        }

        static constexpr TRect XYWH(TPoint<COORD> topLeft, int width, int height) { return XYWH(topLeft.x, topLeft.y, width, height); }
        
        static constexpr TRect XYWH(int x, int y, int width, int height) {
            return TRect{x, y, width, height};
        }

        static constexpr TRect Centered(COORD w, COORD h, COORD maxWidth, COORD maxHeight) {
            return TRect{(maxWidth - w) / 2, (maxHeight - h) / 2, w, h};
        }

    }; // rckid::TRect<>

    using Rect = TRect<Coord>;
    using Point = TPoint<Coord>;

} // namespace rckid