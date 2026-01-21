#pragma once

#include <platform.h>

namespace rckid {

    using Coord = int16_t;

    class Point {
    public:
        Coord x = 0;
        Coord y = 0;

        constexpr Point() = default;
        constexpr Point(Coord x_, Coord y_): x{x_}, y{y_} {}

    }; // rckid::Point

    class Point3D {
    public:
        Coord x = 0;
        Coord y = 0;
        Coord z = 0;

        constexpr Point3D() = default;
        constexpr Point3D(Coord x_, Coord y_, Coord z_): x{x_}, y{y_}, z{z_} {}
    }; // rckid::Point3D



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

        constexpr Coord width() const { return w; }
        constexpr Coord height() const { return h; }

        constexpr Rect() = default;

        static constexpr Rect WH(Coord width, Coord height) {
            return Rect{0, 0, width, height};
        }

        static constexpr Rect XYWH(Coord x, Coord y, Coord width, Coord height) {
            return Rect{x, y, width, height};
        }

    private:
        constexpr Rect(Coord x_, Coord y_, Coord w_, Coord h_): x{x_}, y{y_}, w{w_}, h{h_} {}

    }; // rckid::Rect

    inline Writer & operator << (Writer & w, Rect const & r) {
        w << "Rect[" << r.x << ", " << r.y << ", " << r.w << ", " << r.h << "]";
        return w;
    }


} // namespace rckid