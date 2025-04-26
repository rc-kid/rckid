#pragma once

#include "../graphics/geometry.h"
#include "../graphics/surface.h"
#include "../graphics/bitmap.h"

namespace rckid::ui {

    /** Basic UI element.
     
        Each UI element has its own position and defines the rendering of a single column of pixels, which allows very flexible rendering of many UI elements with very little memory footprint as only a single column buffer is necessaary regardless of the number of elements.

        All UI elements always render to full RGB 565 16bpp pixel arrays for simplicity. 
     */
    class Widget {
    public:
        using Surface = rckid::Surface<16>;

        virtual ~Widget() = default;

        Coord x() const { return x_; }
        Coord y() const { return y_; }
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

        /** Renders the widget directly to the provided bitmap at given coordinates. 
         */
        void renderToBitmap(Coord x, Coord y, Bitmap<ColorRGB> & bmp) {
            Coord numPixels = std::min(height(), bmp.height() - y);
            for (Coord column = std::min(width(), bmp.width() - x); column >= 0; --column) {
                uint16_t * buffer = bmp.columnPixels(column);
                renderColumn(column, buffer + y, y, numPixels);
            }
        }

        /** Update method that can adjust the widget before drawing. 
         */
        virtual void update() {}

        /** If the widget offers any interactivity, calling this method during the app's update call will allow it to respond to the user inputs. Note though that it is the UI itself does not deal with focus, etc. and the app must determine which widget(s) to call this method on.
         */
        virtual void processEvents() {}

    protected:

        Widget() = default;

        Widget(Coord x, Coord y):
            x_{x}, y_{y} {
        }

        Widget(Rect rect): x_{rect.x}, y_{rect.y}, w_{rect.w}, h_{rect.h} {}

        virtual void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) = 0;

        /** Called when the widget is resized so that child classes can override and react to the change such as repositioning their contents. 
         */
        virtual void resize() {}

        /** Renders given child. 
         
            This is a simple matter of adjusting the rendering parameters for the child widget and calling its renderColumn method if there us anything to render.
         */
        void renderChild(Widget * w, Coord column, uint16_t * buffer, Coord starty, Coord numPixels) {
            adjustRenderParams(w->rect(), column, buffer, starty, numPixels);
            if (numPixels != 0) 
                w->renderColumn(column, buffer, starty, numPixels);
        }

        /** Renders the given child at coordinates with given offset.
         */
        void renderChild(Widget *w, Coord column, uint16_t * buffer, Coord starty, Coord numPixels, Point offset) {
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
            if (rect.top() > numPixels || rect.bottom() < starty) {
                numPixels = 0;
                return;
            }
            // adjust the starty and numPixels for the child's column render and update the buffer's start to match. First determine the start of rendering in child's coordinates. This is zero usually, unless the start of the widget is below our own starty, in which case it has to be adjusted.
            Coord wStart = rect.top() >= starty ? 0 : starty - rect.top();
            ASSERT(wStart >= 0 && wStart < rect.height());
            // knowing the start of actual rendering in the child, advance the buffer in the appropriate number of pixels 
            Coord bufferAdvance = rect.top() - starty + wStart;
            ASSERT(bufferAdvance <= numPixels);
            buffer += bufferAdvance;
            // and adjust the number of pixels to render, which is the minimum of the child's height and the available pixels in the buffer
            numPixels = std::min(numPixels - bufferAdvance, rect.height() - wStart);
            // and the column
            column -= rect.left();
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

        Coord x_ = 0;
        Coord y_ = 0;
        Coord w_ = 50;
        Coord h_ = 50;
    }; // rcikd::ui::Element

} // namespace rckid::ui