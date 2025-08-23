#pragma once

#include "widget.h"

namespace rckid::ui {

    class ScrollView : public Widget {
    public:

        Coord offsetLeft() const { return offsetLeft_; }
        Coord offsetTop() const { return offsetTop_; }
        Point offset() const { return Point{offsetLeft_, offsetTop_}; }

        void setOffsetLeft(Coord offset) { offsetLeft_ = offset; }
        void setOffsetTop(Coord offset) { offsetTop_ = offset; }
        void setOffset(Point offset) { offsetLeft_ = offset.x; offsetTop_ = offset.y; }

    protected:

        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) {
            Widget::renderColumn(column + offsetLeft_, buffer, starty + offsetTop_, numPixels);
        }
    
    private:
        Coord offsetLeft_ = 0;
        Coord offsetTop_ = 0;
    }; // rckid::ui::ScrollView
}