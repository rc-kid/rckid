#pragma once

namespace rckid {

    class Point {
    public:
        Point() = default;
        constexpr Point(int x, int y): x_{x}, y_{y} {}

        constexpr static Point origin() { return Point{0,0}; }

        constexpr int x() const { return x_; }
        constexpr int y() const { return y_; }

        Point operator + (Point other) const { return Point{x_ + other.x_, y_ + other.y_}; }
        Point operator - (Point other) const { return Point{x_ - other.x_, y_ - other.y_}; }
        
    private:
        int x_ = 0;
        int y_ = 0;
    }; 

    class Rect {
    public:
        static constexpr Rect WH(int width, int height) {
            return Rect{Point{0,0}, Point{width, height}};
        }

        static constexpr Rect XYWH(Point topLeft, int width, int height) { return XYWH(topLeft.x(), topLeft.y(), width, height); }
        
        static constexpr Rect XYWH(int x, int y, int width, int height) {
            return Rect{
                Point{x, y}, 
                Point{x + width, y + height}
            };
        }

        constexpr int top() const { return topLeft_.y(); }
        constexpr int left() const { return topLeft_.x(); }
        constexpr int bottom() const { return bottomRight_.y(); }
        constexpr int right() const { return bottomRight_.x(); }
        constexpr int width() const { return right() - left(); }
        constexpr int height() const { return bottom() - top(); }
        constexpr Point const & topLeft() const { return topLeft_; }
        constexpr Point const & bottomRight() const { return bottomRight_; }

        /** Rectangle intersection.
         */
        Rect operator && (Rect const & other) const {
            int l = std::max(left(), other.left());
            int r = std::min(right(), other.right());
            int t = std::max(top(), other.top());
            int b = std::min(bottom(), other.bottom());
            if (r < l)
                r = l;
            if (b < t)
                b = t;
            return XYWH(l, t, r - l, b - t);
        }
    private:

        constexpr Rect(Point tl, Point br): topLeft_{tl}, bottomRight_{br} {}

        Point topLeft_;
        Point bottomRight_;
    }; 


} // namespace rckid