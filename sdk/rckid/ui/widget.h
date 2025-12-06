#pragma once

#include "../graphics/geometry.h"
#include "../graphics/pixel_surface.h"
#include "../graphics/bitmap.h"

namespace rckid::ui {

    /** Basic UI element.
     
        Each UI element has its own position and defines the rendering of a single column of pixels, which allows very flexible rendering of many UI elements with very little memory footprint as only a single column buffer is necessaary regardless of the number of elements.

        All UI elements always render to full RGB 565 16bpp pixel arrays for simplicity. 
     */
    class Widget {
    public:
        using Surface = rckid::PixelSurface<16>;

        /** Delete all children that are allocated on heap.
         
            Non-heap children (presumably coming from stack) are not deallocated by the widget itself so that stack and heap based allocation is supported for the UI trees.
         */
        virtual ~Widget() {
            clearChildren();
        }

        Coord left() const { return x_; }
        Coord top() const { return y_; }
        Coord bottom() const { return y_ + h_; }
        Coord right() const { return x_ + w_; }
        Point pos() const { return Point{x_, y_}; }

        Coord width() const { return w_; }
        Coord height() const { return h_; }
        Rect rect() const { return Rect::XYWH(x_, y_, w_, h_); }

        void setX(Coord x) {
            if (x_ == x)
                return;
            x_ = x;
            resize();
        }

        void setY(Coord y) { 
            if (y_ == y)
                return;
            y_ = y;
            resize();
        }

        void setPos(Point pos) { setPos(pos.x, pos.y); }

        void setPos(Coord x, Coord y) { 
            if (x_ == x && y_ == y)
                return;
            x_ = x; 
            y_ = y; 
            resize();
        }

        void setWidth(Coord w) { 
            if (w_ != w) {
                w_ = w;
                resize();
            }
        }
        
        void setHeight(Coord h) {
            if (h_ != h) {
                h_ = h;
                resize();
            }
        }
        
        void setRect(Rect rect) { 
            x_ = rect.x; 
            y_ = rect.y; 
            if (w_ != rect.w || h_ != rect.h) {
                w_ = rect.w; 
                h_ = rect.h; 
                resize();
            }
        }

        template<typename T>
        T * addChild(T * child) {
            children_.push_back(child);
            return child;
        }

        void clearChildren() {
            for (auto w : children_)
                delete w;
            children_.clear();
        }

        std::vector<Widget *> const & children() const { return children_; }

        Widget * lastChild() const {
            if (children_.size() == 0)
                return nullptr;
            return children_.back();
        }

        bool visible() const { return visible_; }
        
        void setVisible(bool value) {
            visible_ = value;
        }

        /** Returns true if the widget is focused. 
         
            Focused widget will have its processEvents() method called during update. 
         */
        bool focused() const { return focused_ == this; }

        /** Focuses the widget on which the method is called. 
         */
        void focus() { focused_ = this; }

        /** Returns the currenly focused widget, or nullptr if no widget is in focus. 
         */
        static Widget * focusedWidget() { return focused_; }

        /** Update method
         
            Called while the UI is being re-drawn, i.e. without the drawing lock. This method can be used to update any state that the widget may have before the draw() method is called. Generally this method precomputes data that will later be used by drawing.
         */
        virtual void update() {
            for (auto w : children_)
                w->update();
            if (focused())
                processEvents();
        }

        /** Draw method. 
         
            The draw method is called by the renderer before any columns are rendered to the sceen, but with lock on the drawing data, which makes it the place to adjust widget parameters before the actual rendering. 
         */
        virtual void draw() {
            if (! visible_)
                return;
            for (auto w : children_)
                w->draw();
        }

        /** If the widget offers any interactivity, calling this method during the app's update call will allow it to respond to the user inputs. 
         
            Called automatically if a widget is focused, but may be called manually as well. 
         */
        virtual void processEvents() { }

        /** Widget simply renders columns of all child elements in the order they are defined in the list of children, i.e. the earier children can be overdrawn with the later ones. Override this function in child classes to provide the widget specific rendering. 
         */
        virtual void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) {
            for (auto w : children_)
                renderChild(w, column, buffer, starty, numPixels);
        }

    protected:

        Widget() = default;

        Widget(Coord x, Coord y):
            x_{x}, y_{y} {
        }

        Widget(Rect rect): x_{rect.x}, y_{rect.y}, w_{rect.w}, h_{rect.h} {}


        /** Called when the widget is resized so that child classes can override and react to the change such as repositioning their contents. 
         */
        virtual void resize() {}

        /** Renders given child. 
         
            This is a simple matter of adjusting the rendering parameters for the child widget and calling its renderColumn method if there us anything to render.
         */
        void renderChild(Widget * w, Coord column, uint16_t * buffer, Coord starty, Coord numPixels) {
            if (!w->visible_)
                return;
            adjustRenderParams(w->rect(), column, buffer, starty, numPixels);
            if (numPixels != 0) 
                w->renderColumn(column, buffer, starty, numPixels);
        }

        /** Renders the given child at coordinates with given offset.
         */
        void renderChild(Widget *w, Coord column, uint16_t * buffer, Coord starty, Coord numPixels, Point offset) {
            if (!w->visible_)
                return;
            Rect rect = w->rect();
            rect.x += offset.x;
            rect.y += offset.y;
            adjustRenderParams(rect, column, buffer, starty, numPixels);
            if (numPixels != 0) 
                w->renderColumn(column, buffer, starty, numPixels);
        }

        /** Adjusts the rendering parameters for given rectangle within the widget. 
         
            If no rendering should occur, sets the number of pixels to be rendered to zero. In this case no other arguments should be considered valid after the call.
         */
        void adjustRenderParams(Rect rect, Coord & column, uint16_t * & buffer, Coord & starty, Coord & numPixels) {
            // don't render if the child widget's rectangle does not intersect with the column to be rendered (no horizontal intersection)
            if (column < rect.left() || column >= rect.right()) {
                numPixels = 0;
                return;
            }
            // don't render if the rect does not intersect with the buffer beginning and the available number of pixels (no vertical intersection)
            if (rect.top() >= (numPixels + starty) || rect.bottom() <= starty) {
                numPixels = 0;
                return;
            }
            // adjust the starty and numPixels for the child's column render and update the buffer's start to match. First determine the start of rendering in child's coordinates. This is zero usually, unless the start of the widget is below our own starty, in which case it has to be adjusted.
            Coord wStart = rect.top() >= starty ? 0 : starty - rect.top();
            ASSERT(wStart >= 0 && wStart < rect.height());
            // knowing the start of actual rendering in the child, advance the buffer in the appropriate number of pixels 
            Coord bufferAdvance = rect.top() - starty + wStart;
            ASSERT(bufferAdvance < numPixels);
            buffer += bufferAdvance;
            // and adjust the number of pixels to render, which is the minimum of the child's height and the available pixels in the buffer
            numPixels = std::min(numPixels - bufferAdvance, rect.height() - wStart);
            // and the column
            column -= rect.left();
            starty = wStart;
        }

        /** Shorthand function that justifies the inner rectangle within inside the current widget. 
         */
        Point justifyRectangle(Rect inner, HAlign ha, VAlign va) {
            return justifyRectangle(Rect::WH(width(), height()), inner, ha, va);
        }

        /** Justifies the inner widget according to the outer parent and returns the inner widget's topleft coordinates. 
         */
        Point justifyRectangle(Rect parent, Rect inner, HAlign ha, VAlign va) {
            Point res;
            switch (ha) {
                case HAlign::Left:
                    res.x = parent.left();
                    break;
                case HAlign::Center:
                    res.x = parent.left() + (parent.width() - inner.width()) / 2;
                    break;
                case HAlign::Right:
                    res.x = parent.right() - inner.width();
                    break;
                case HAlign::Custom:
                    res.x = inner.x;
                    break;
            }
            switch (va) {
                case VAlign::Top:
                    res.y = parent.top();
                    break;
                case VAlign::Center:
                    res.y = parent.top() + (parent.height() - inner.height()) / 2;
                    break;
                case VAlign::Bottom:
                    res.y = parent.bottom() - inner.height();
                    break;
                case VAlign::Custom:
                    res.y = inner.y;
                    break;
            }
            return res;
        }

        bool visible_ = true;

        Coord x_ = 0;
        Coord y_ = 0;
        Coord w_ = 50;
        Coord h_ = 50;

        std::vector<Widget *> children_;

        static inline Widget * focused_ = nullptr;

    }; // rcikd::ui::Widget

} // namespace rckid::ui