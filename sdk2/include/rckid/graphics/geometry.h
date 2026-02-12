#pragma once

#include <platform.h>
#include <platform/writer.h>

namespace rckid {

    /** Coordinates in RCKid use platform's native integer type, even though much smaller range is generally required, this makes arithemtic operations faster and simpler (no typecasts). 
     */
    using Coord = int32_t;

    /** Maps coordinates into a 2D array into one dimensional array in a column first manner where the first indes is mapped to the last column, first row. This mapping is tailored to the native display rendering where column by column rendering means simply incrementing the array index after the first one.  
     */
    constexpr inline uint32_t mapIndexColumnFirst(Coord x, Coord y, Coord width, Coord height) {
        return (width - x - 1) * height + y;
    }

    /** Horizontal alignment. 
     
        Used for widget placement and resizing. Can either be manual (in which case no automatic adjustments will be done), or the usual left, center, and right values.
     */
    enum class HAlign {
        Left,
        Center,
        Right,
        Manual,
    };

    /** Vertical alignment. 
     
        Used for widget placement and resizing. Can either be manual (in which case no automatic adjustments will be done), or the usual top, center, and bottom values.
     */
    enum class VAlign {
        Top,
        Center,
        Bottom,
        Manual,
    };

    /** Direction. 
     */
    enum class Direction {
        Left,
        Right,
        Up,
        Down,
    };

    class Point {
    public:
        Coord x = 0;
        Coord y = 0;

        constexpr Point() = default;
        constexpr Point(Coord x_, Coord y_): x{x_}, y{y_} {}

        Point operator + (Point other) const {
            return Point{x + other.x, y + other.y};
        }

        Point operator - (Point other) const {
            return Point{x - other.x, y - other.y};
        }

    }; // rckid::Point


    inline Point operator * (Direction dir, Coord magnitude) {
        switch (dir) {
            case Direction::Left:
                return Point{-magnitude, 0};
            case Direction::Right:
                return Point{magnitude, 0};
            case Direction::Up:
                return Point{0, -magnitude};
            case Direction::Down:
                return Point{0, magnitude};
            default:
                UNREACHABLE;
        }
    }

    inline Point operator * (Coord magnitude, Direction dir) {
        return dir * magnitude;
    }

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

        void setTopLeft(Point pos) {
            x = pos.x;
            y = pos.y;
        }

        constexpr Coord width() const { return w; }
        constexpr Coord height() const { return h; }

        /** Returns true if the other rectangle fits completely inside this one.
         */
        bool contains(Rect const & other) const {
            return left() <= other.left() && top() <= other.top() && right() >= other.right() && bottom() >= other.bottom();
        }


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

    inline Writer operator << (Writer w, Rect const & r) {
        w << "Rect[" << r.x << ", " << r.y << ", " << r.w << ", " << r.h << "]";
        return w;
    }


} // namespace rckid