#pragma once

#include "widget.h"

namespace rckid::ui {

    class ScrollView : public Widget {
    public:

        ScrollView() = default;
        ScrollView(Rect pos): Widget{pos} {}

        Coord offsetLeft() const { return offsetLeft_; }
        Coord offsetTop() const { return offsetTop_; }
        Point offset() const { return Point{offsetLeft_, offsetTop_}; }

        void setOffsetLeft(Coord offset) { offsetLeft_ = offset; }
        void setOffsetTop(Coord offset) { offsetTop_ = offset; }
        void setOffset(Point offset) { offsetLeft_ = offset.x; offsetTop_ = offset.y; }

        void scrollTopLeft() {
            Coord minX = std::numeric_limits<int16_t>::max();
            Coord minY = std::numeric_limits<int16_t>::max();
            for (auto w : children_) {
                if (w->x() < minX)
                    minX = w->x();
                if (w->y() < minY)
                    minY = w->y();
            }
            setOffset(Point{minX, minY});
        }

        void scrollBottomLeft() {
            Coord minX = std::numeric_limits<int16_t>::max();
            Coord maxY = std::numeric_limits<int16_t>::min();
            for (auto w : children_) {
                if (w->x() < minX)
                    minX = w->x();
                if (w->y() + w->height() > maxY)
                    maxY = w->y() + w->height();
            }
            setOffset(Point{minX, maxY - height()});
        }

    protected:

        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) {
            Widget::renderColumn(column + offsetLeft_, buffer, starty + offsetTop_, numPixels);
        }
    
    private:
        Coord offsetLeft_ = 0;
        Coord offsetTop_ = 0;
    }; // rckid::ui::ScrollView
}