#pragma once

#include <vector>

#include <rckid/rckid.h>
#include <rckid/memory.h>
#include <rckid/graphics/color.h>
#include <rckid/graphics/geometry.h>
#include <rckid/ui/with.h>

namespace rckid::ui {

    class Widget {
    public:

        Widget() = default;

        Widget(Rect rect): rect_{rect} {
            if (rect_.w < 0 || rect_.h < 0) {
                LOG(LL_ERROR, "Widget rectangle has negative size " << rect);
                if (rect_.w < 0)
                    rect_.w = 0;
                if (rect_.h < 0)
                    rect_.h = 0;
            }
        }

        virtual ~Widget() = default;

        Rect rect() const { return rect_; }

        /** Returns the width of the widget.
         
            Width cannot be negative.
         */
        Coord width() const { 
            return rect_.width(); 
        }

        /** Returns the height of the widget. 
         
            Height cannot be negative.
         */
        Coord height() const { 
            return rect_.height(); 
        }

        Point position() const {
            return rect_.topLeft();
        }

        void setPosition(Point pos) {
            rect_.setTopLeft(pos);
        }

        bool visible() const { return visible_; }

        void setVisible(bool value) { 
            visible_ = value; 
        }

        template<typename T>
        T * addChild(T * child) {
            children_.push_back(unique_ptr<Widget>(child));
            return child;
        }

        template<typename T>
        T * addChild(with<T> const & child) {
            return addChild(static_cast<T*>(child));
        }

        /** Renders vertical column of the the widget to given color buffer. 
         */
        virtual void renderColumn(Coord column, Color::RGB565 * buffer, Coord starty, Coord numPixels) {

        }

    protected:

        virtual void onRender() {
            for (auto & child : children_)
                if (child->visible())
                    child->onRender();
        }

    private:

        template<typename T>
        friend class App;

        Rect rect_;
        bool visible_ = true;

        std::vector<unique_ptr<Widget>> children_;

    }; // ui::Widget


    struct SetPosition {
        Point pos;

        SetPosition(Point p) : pos{p} {}
        SetPosition(Coord x, Coord y) : pos{x, y} {}

        void operator () (Widget * w) const { w->setPosition(pos); }
    };

    struct SetVisibility {
        bool value;

        void operator () (Widget * w) const { w->setVisible(value); }
    }; 



} // namespace rckid