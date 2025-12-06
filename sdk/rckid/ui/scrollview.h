#pragma once

#include "../utils/timer.h"
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

        /** Scrolls the given widget to view.
         
            Scrolls the object to full visibility. Uses the shortest possible scroll path. Does nothing if already fully visible. 
         */
        void scrollToView(Widget * w) {
            deltaY_ = 0;
            if (w->top() < offsetTop_)
                deltaY_ =  w->top() - offsetTop_;
            if (w->bottom() > offsetTop_ + height())
                deltaY_ = - (offsetTop_ + height() - w->bottom());
            if (deltaY_ != 0) {
                aux_ = offsetTop_;
                a_.start();
            }
        }

        void scrollTopLeft() {
            Coord minX = std::numeric_limits<int16_t>::max();
            Coord minY = std::numeric_limits<int16_t>::max();
            for (auto w : children_) {
                if (w->left() < minX)
                    minX = w->left();
                if (w->top() < minY)
                    minY = w->top();
            }
            setOffset(Point{minX, minY});
        }

        void scrollBottomLeft() {
            Coord minX = std::numeric_limits<int16_t>::max();
            Coord maxY = std::numeric_limits<int16_t>::min();
            for (auto w : children_) {
                if (w->left() < minX)
                    minX = w->left();
                if (w->bottom() > maxY)
                    maxY = w->bottom();
            }
            setOffset(Point{minX, maxY - height()});
        }

        // TODO this is not sth we want to run manually, instead would be nice if this is integrated into the widgets
        void updateChildRect() {
            if (children_.size() == 0) {
                childRect_ = Rect::WH(0,0);
                return;
            }
            Coord minX = std::numeric_limits<Coord>::max();
            Coord minY = std::numeric_limits<Coord>::max();
            Coord maxX = std::numeric_limits<Coord>::min();
            Coord maxY = std::numeric_limits<Coord>::min();
            for (auto w : children_) {
                if (w->left() < minX)
                    minX = w->left();
                if (w->top() < minY)
                    minY = w->top();
                if (w->right() > maxX)
                    maxX = w->right();
                if (w->bottom() > maxY)
                    maxY = w->bottom();
            }
            childRect_ = Rect::XYWH(minX, minY, maxX - minX, maxY - minY);
        }

    protected:

        void update() override {
            displayWaitUpdateDone();
            Widget::update();
/*            
            if (dir_ == Direction::None) {
                if (btnDown(Btn::Down)) {
                    aux_ = offsetTop_;
                    dir_ = Direction::Down;
                    a_.start();
                }
                if (btnDown(Btn::Up)) {
                    aux_ = offsetTop_;
                    dir_ = Direction::Up;
                    a_.start();
                }
            }
*/
        }

        void draw() override {
            if (deltaY_ != 0) {
                if (a_.update()) {
                    offsetTop_ = aux_ + deltaY_;
                    deltaY_ = 0;
                    a_.stop();
                } else {
                    Coord delta = interpolation::linear(a_, 0, deltaY_).round();
                    offsetTop_ = aux_ + delta;
                }
            }
            Widget::draw();
        }


        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            Widget::renderColumn(column + offsetLeft_, buffer, starty + offsetTop_, numPixels);
        }
    
    private:
        Coord offsetLeft_ = 0;
        Coord offsetTop_ = 0;
        Coord aux_ = 0;
        Coord deltaY_ = 0;
        Rect childRect_;
        Timer a_{500};

    }; // rckid::ui::ScrollView
}