#pragma once

#include <cstdint>

namespace rckid {

    using Coord = int;

    class Point {
    public:
        Coord x = 0;
        Coord y = 0;

        Point() = default;
        constexpr Point(int x, int y): x{x}, y{y} {}

        constexpr static Point origin() { return Point{0,0}; }

        constexpr Point operator + (Point other) const { return Point{x + other.x, y + other.y}; }
        constexpr Point operator - (Point other) const { return Point{x - other.x, y - other.y}; }
        
        constexpr bool operator == (Point other) const { return x == other.x && y == other.y; }

    }; // rckid::Point

    class Rect {
    public:
        Coord x = 0;
        Coord y = 0;
        Coord w = 0;
        Coord h = 0;

        constexpr Coord top() const { return y; }
        constexpr Coord left() const { return x; }
        constexpr Coord bottom() const { return y + h; }
        constexpr Coord right() const { return x + w; }

        constexpr Point topLeft() const { return Point{x, y}; }
        constexpr Point bottomRight() const { return Point{right(), bottom()}; }

        static constexpr Rect WH(int width, int height) {
            return Rect{0, 0, width, height};
        }

        static constexpr Rect XYWH(Point topLeft, int width, int height) { return XYWH(topLeft.x, topLeft.y, width, height); }
        
        static constexpr Rect XYWH(int x, int y, int width, int height) {
            return Rect{x, y, width, height};
        }

    }; // rckid::Rect


} // namespace rckid