#pragma once

#include <vector>

#include <rckid/rckid.h>
#include <rckid/memory.h>
#include <rckid/graphics/color.h>
#include <rckid/graphics/geometry.h>

namespace rckid::ui {

    class Widget {
    public:

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

        bool visible() const { return visible_; }


        template<typename T>
        T * addChild(T * child) {
            children_.push_back(unique_ptr<Widget>(child));
            return child;
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


} // namespace rckid