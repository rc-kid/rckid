#pragma once

#include <vector>
#include <algorithm>

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
         
            Takes the column that should be rendered (relative to the widget's left edge), the row at which the rendering should start (relative to widget's top), pointer to the color buffer where the rendering should happen, and the number of pixels to render.

            Widget's base implementation does not do any own rendering, but simply renders all of its children. Specific widgets should override this method to provide their own rendering logic, then call the base class implementation to render the children.
         */
        virtual void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) {
            ASSERT(verifyRenderParams(width(), height(), column, startRow, numPixels));
            for (auto & child : children_)
                renderChildColumn(child.get(), column, startRow, buffer, numPixels);
        }

    protected:

        virtual void onRender() {
            for (auto & child : children_)
                if (child->visible())
                    child->onRender();
        }

        /** Renders column of given child. 
         
            Adjusts own rendering parameters to the child's rectangle and calls its renderColumn method. If the child is *not* visible, returns immediately.
         */
        void renderChildColumn(Widget * w, Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) {
            if (!w->visible_)
                return;
            adjustRenderParams(w->rect(), column, startRow,  buffer, numPixels);
            if (verifyRenderParams(w->width(), w->height(), column, startRow, numPixels))
                w->renderColumn(column, startRow, buffer, numPixels);
        }

        /** Verifies that the rendering parameters are valid for given width & height. 
         
            This simply means that number of pixels to render is positive, the start row and the number of pixels to draw do not extend the height and the column is within the given width. 
         */
        static bool verifyRenderParams(Coord ownWidth, Coord ownHeight, Coord column, Coord startRow, Coord numPixels) {
            ASSERT(ownWidth >= 0);
            ASSERT(ownHeight >= 0);
            // nothing to draw
            if (numPixels <= 0)
                return false;
            // too much to draw
            if (numPixels + startRow > ownHeight)
                return false;
            // column outside of range
            if (column < 0 || column >= ownWidth)
                return false;
            // start row outside of range
            if (startRow < 0)
                return false;
            // otherwise all is good
            return true;
        }

        /** Adjusts the rendeing parameters so that they'll be relative to the given rectangle.
         
            This is simple process for the column, where we simply adjust the column based on the rectangle's position and deal with out of bound values later as the new column value can be negative, or greater than the rectangle's width it such case. 

            The rest of the render parameters are independent of the column and need to be adjusted together. 



            TODO can the row calculations be cached for each frame? 
         */
        static void adjustRenderParams(Rect rect, Coord & column, Coord & startRow, Color::RGB565 * & buffer, Coord & numPixels) {
            // column is simple as it is independent from the rest of params that concern the row alone
            column = column - rect.left();
            // determine the distance by which we have to advance the buffer first. 
            Coord rowDelta = rect.top() - startRow;
            // if rowDelta is positive, this means we must advance the buffer by the delta, set startRow to 0 (we will start at the beginning of the rectangle) and decrase the number of pixels to render to min of remaining pixels after buffer adjustment and rectangle height
            if (rowDelta >= 0) {
                buffer += rowDelta;
                startRow = 0;
                numPixels = std::min(static_cast<Coord>(numPixels - rowDelta), rect.height());
            // if rowDelta is negative (meaning the rectangle's top is above the startRow) we do not need to advance the buffer, but must adjust startRow (which will become absolute value of the delta). Number of pixels to render will become 
            } else {
                startRow = -rowDelta;
                numPixels = std::min(numPixels, static_cast<Coord>(rect.height() - startRow));
            }
        }

    private:

        template<typename T>
        friend class App;

        Rect rect_;
        bool visible_ = true;

        std::vector<unique_ptr<Widget>> children_;

    }; // ui::Widget


    /** Operator for setting widget's position.
     */
    struct SetPosition {
        Point pos;

        SetPosition(Point p) : pos{p} {}
        SetPosition(Coord x, Coord y) : pos{x, y} {}

        void operator () (Widget * w) const { w->setPosition(pos); }
    };

    /** Operator for setting widget's visibility. 
     */
    struct SetVisibility {
        bool value;

        void operator () (Widget * w) const { w->setVisible(value); }
    }; 



} // namespace rckid