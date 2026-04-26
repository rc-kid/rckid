#pragma once

#include <vector>
#include <algorithm>

#include <rckid/rckid.h>
#include <rckid/memory.h>
#include <rckid/graphics/color.h>
#include <rckid/graphics/font.h>
#include <rckid/graphics/geometry.h>
#include <rckid/ui/with.h>
#include <rckid/ui/style.h>

namespace rckid::ui {

    class Animation;

    class Widget {
    public:

        class AnimationBuilder {
        public:
            AnimationBuilder operator << (Animation * animation);
        private:
            friend class Widget;

            AnimationBuilder(Widget * widget): target_{widget} {}

            Widget * target_;

        }; // Widget::AnimationBuilder

        Widget() = default;

        virtual ~Widget() {
            // cancel any animations attached to the widget
            cancelAnimations();
        }

        virtual void applyStyle(Style const & style) {
            animationSpeed_ = style.animationSpeed();
        }

        Rect rect() const { return rect_; }
        Point position() const { return Point{rect_.x, rect_.y}; }
        Coord x() const { return rect_.x; }
        Coord y() const { return rect_.y; }

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
        
        void setRect(Rect rect) {
            rect_ = rect;
            if (rect_.w < 0 || rect_.h < 0) {
                LOG(LL_ERROR, "Widget rectangle has negative size " << rect);
                if (rect_.w < 0)
                    rect_.w = 0;
                if (rect_.h < 0)
                    rect_.h = 0;
            }
            onResize();
        }

        Widget * parent() const { return parent_; }

        bool visible() const { return visible_; }

        void setVisibility(bool value) { 
            visible_ = value; 
        }

        bool visibleInParent() const {
            if (!visible_)
                return false;
            if (parent_ == nullptr)
                return true;
            return ! Rect::WH(parent_->width(), parent_->height()).intersectWith(rect()).empty();
        }

        uint32_t animationSpeed() const { return animationSpeed_; }

        void setAnimationSpeed(uint32_t animationSpeed) { 
            animationSpeed_ = animationSpeed; 
        }

        /** Animates entrance of widget contents. 
         
            Takes all immediate children of the widgets and animates them using flyIn animation with the specified distance. The widgets are delayed based on their y coordinate. This method is useful when called on a root widget for an application to easily animate its entire contents on app start.
         */
        void flyIn(Point distance);

        /** Animates exit of widget contents. 
         
            Takes all immediate children of the widgets and animates them using flyOut animation with the specified distance. The widgets are delayed based on their y coordinate. Very useful for root widgets to animate app ui fly out when the app closes.
         */
        void flyOut(Point distance);

        /** Shorthand for flyIn animation for widget contents with the distance being from the top of the screen (contents come from above the screen).
         */
        void flyIn() {
            flyIn(Point{0, -height()});
        }

        /** Shorthand for flyOut animation for widget contents with the distance being from the top of the screen (contents disappear above screen)
         */
        void flyOut() {
            flyOut(Point{0, -height()});
        }

        template<typename T>
        with<T> addChild(T * child) {
            ASSERT(child->parent_ == nullptr);
            child->parent_ = this;       
            child->applyStyle(Style::defaultStyle());
            children_.push_back(unique_ptr<Widget>(child));
            return with<T>(child);
        }

        virtual bool idle() const { return activeAnimations_ == 0; }

        /** Returns animation builder for the current widget. 
          
            All animations created through it will be registered to the widget (and hence affect its idle status).
         */
        AnimationBuilder animate() { return AnimationBuilder{this}; }

        /** Cancels all animations registered on the widget. 
         */
        void cancelAnimations();

        /** Clears all children of the widget.
         */
        void clearChildren() {
            children_.clear();
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

        /** Can be called to make the widget process any relevant input events.
         
            This can be called manually, or if the widget is focused is called automatically by the application.
         */
        virtual void processEvents() {
            // nop
        }

        /** Returns true if the current widget is focused. 
         */
        bool focused() const { return focused_; }

        /** Performs the pre-rednering essentials required for UI elements.
         
            This function must be called before any widgets are being drawn as it updates any existing animations and calls onRender for the non owned widgets (such as the Header bar). By default the RootWidget calls this automatically, so any ui::App classes do not have to do anything. But if an app uses custom rendering loop *and* uses UI elements as well, it must call this method before any frame.
         */
        static void renderEssentials();

    protected:

        virtual void onRender() {
            for (auto & child : children_)
                if (child->visible())
                    child->onRender();
        }

        virtual void onResize() {
            // nop
        }

        /** Called when the widget transitions to idle state, i.e. has no animations attached to it.
         */
        virtual void onIdle() {
            // nop
        }

        virtual void onFocus() {
            ASSERT(focused_ == false);
            focused_ = true;
            // nop
        }

        virtual void onBlur() {
            ASSERT(focused_ == true);
            focused_ = false;
            // nop
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

        /** Adjusts the rendering parameters so that they'll be relative to the given rectangle.
         
            This is simple process for the column, where we simply adjust the column based on the rectangle's position and deal with out of bound values later as the new column value can be negative, or greater than the rectangle's width it such case. 

            The rest of the render parameters are independent of the column and need to be adjusted together. We determine the row offset, based on which the buffer, starRow and number of pixels to draw is adjusted. 

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

        void adjustRenderParams(Point offset, Coord & column, Coord & startRow, Color::RGB565 * & buffer, Coord & numPixels) {
            column -= offset.x;
            startRow -= offset.y;
            if (startRow < 0) {
                buffer -= startRow;
                numPixels += startRow; // start row is really negative
                startRow = 0;
            }
        }

        /** Helper function that allows triggering onRender() in other widgets w/o making the function public. This is very usefuyl for wrapper widgets, such as Launcher::BorrowedCarousel, that do not have their own rendering logic, but simply delegate the rendering to the wrapped widget.
         */
        static void triggerOnRender(Widget * w) { w->onRender();}

        static void triggerProcessedEvents(Widget * w) { w->processEvents(); }

    private:

        template<typename T>
        friend class App;

        friend class Animation;

        uint32_t activeAnimations_ = 0;

        Rect rect_;
        Widget * parent_ = nullptr;
        bool visible_ = true;
        bool focused_ = false;
        uint32_t animationSpeed_ = RCKID_DEFAULT_ANIMATION_DURATION_MS;

        std::vector<unique_ptr<Widget>> children_;

    }; // ui::Widget

    /** Sets the full position rectangle of the widget (i.e. position and size).
     */
    struct SetRect {
        Rect rect;
        SetRect(Rect rect): rect{rect} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetRect sr) {
        w->setRect(sr.rect);
        return w;
    }

    /** Sets position of the widget, leaving its size intact. 
     */
    struct SetPosition {
        Point pos;
        SetPosition(Point pos): pos{pos} {}
        SetPosition(Coord x, Coord y): pos{x,y} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetPosition sp) {
        Rect r = w->rect();
        r.setTopLeft(sp.pos);
        w->setRect(r);
        return w;
    }

    /** Sets width of the widget, leaving other position & size unchanged.
     */
    struct SetWidth {
        Coord const width;
        SetWidth(Coord width): width{width} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetWidth sw) {
        Rect r = w->rect();
        r.w = sw.width;
        w->setRect(r);
        return w;
    }

    /** Sets width of the widget, leaving other position & size unchanged.
     */
    struct SetHeight {
        Coord const height;
        SetHeight(Coord height): height{height} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetHeight sh) {
        Rect r = w->rect();
        r.h = sh.height;
        w->setRect(r);
        return w;
    }

    /** Sets size of the widget without changing its position.
     */
    struct SetSize {
        Coord width;
        Coord height;
        SetSize(Coord width, Coord height): width{width}, height{height} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetSize ss) {
        Rect r = w->rect();
        r.w = ss.width;
        r.h = ss.height;
        w->setRect(r);
        return w;
    }

    /** Sets horizontal alignment of the widget inside its parent. 
     */
    struct AlignHorizontally {
        HAlign align;
        AlignHorizontally(HAlign align): align{align} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, AlignHorizontally ah) {
        ASSERT(w->parent() != nullptr);
        Point pos = w->rect().topLeft();
        switch (ah.align) {
            case HAlign::Left:
                pos.x = 0;
                break;
            case HAlign::Center:
                pos.x = (w->parent()->width() - w->width()) / 2;
                break;
            case HAlign::Right:
                pos.x = w->parent()->width() - w->width();
                break;
            default:
                UNREACHABLE;
        }
        w << SetPosition(pos);
        return w;
    }

    /** Sets vertical alignment of the widget inside its parent.
     */
    struct AlignVertically {
        VAlign align;
        AlignVertically(VAlign align): align{align} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, AlignVertically av) {
        ASSERT(w->parent() != nullptr);
        Point pos = w->rect().topLeft();
        switch (av.align) {
            case VAlign::Top:
                pos.y = 0;
                break;
            case VAlign::Center:    
                pos.y = (w->parent()->height() - w->height()) / 2;
                break;
            case VAlign::Bottom:
                pos.y = w->parent()->height() - w->height();
                break;
            default:
                UNREACHABLE;
        }
        w << SetPosition(pos);
        return w;
    }

    /** Centers the widget horizontally inside its parent. 
     */
    struct CenterHorizontally {};
    template<typename T> 
    inline with<T> operator << (with<T> w, CenterHorizontally) {
        w << AlignHorizontally(HAlign::Center);
        return w;
    }

    /** Centers the widget vertically inside its parent.
     */
    struct CenterVertically {};
    template<typename T> 
    inline with<T> operator << (with<T> w, CenterVertically) {
        w << AlignVertically(VAlign::Center);
        return w;
    }

    /** Centers the widget both horizontally and vertically inside its parent.
     */
    struct Center {};
    template<typename T>
    inline with<T> operator << (with<T> w, Center) {
        w << CenterHorizontally();
        w << CenterVertically();
        return w;
    }

    /** Positions the widget right of the given anchor with provided gap.
     */
    struct RightOf {
        Widget * w;
        Point gap;
        RightOf(Widget * w, Point gap = Point{0, 0}): w{w}, gap{gap} {}
        RightOf(Widget * w, Coord gap): w{w}, gap{Point{gap, 0}} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, RightOf r) {
        w << SetPosition(r.w->position() + r.gap + Point{r.w->width(), 0});
        return w;
    }
    // TODO left of, Above, Below

    /** Sets widget's visibility.
     */
    struct SetVisibility {
        bool visible;
        SetVisibility(bool visible): visible{visible} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetVisibility sv) {
        w->setVisibility(sv.visible);
        return w;
    }

    // common, but not directly widget related properties

    struct SetColor {
        Color color;
        SetColor(Color color): color{color} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetColor sc) {
        w->setColor(sc.color);
        return w;
    }

    struct SetColorGradient {
        Color a;
        Color b;
        SetColorGradient(Color a, Color b): a{a}, b{b} {}
    };

    template<typename T>
    inline with<T> operator << (with<T> w, SetColorGradient scg) {
        w->setColorGradient(scg.a, scg.b);
        return w;
    }
    
    struct SetBg {
        Color color;
        SetBg(Color color): color{color} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetBg sb) {
        w->setBg(sb.color);
        return w;
    }

    struct SetFg {
        Color color;
        SetFg(Color color): color{color} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetFg sf) {
        w->setFg(sf.color);
        return w;
    }


    struct SetHAlign {
        HAlign align;
        SetHAlign(HAlign align): align{align} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetHAlign sha) {
        w->setHAlign(sha.align);
        return w;
    }

    struct SetVAlign {
        VAlign align;
        SetVAlign(VAlign align): align{align} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetVAlign sva) {
        w->setVAlign(sva.align);
        return w;
    }

    struct ApplyStyle {
        Style const & style;
        ApplyStyle(Style const & style): style{style} {}
    };

    template<typename T>
    inline with<T> operator << (with<T> w, ApplyStyle as) {
        w->applyStyle(as.style);
        return w;
    }

    struct SetAnimationSpeed {
        uint32_t speedMs;
        SetAnimationSpeed(uint32_t speedMs): speedMs{speedMs} {}
    };

    template<typename T>
    inline with<T> operator << (with<T> w, SetAnimationSpeed sas) {       
        w->setAnimationSpeed(sas.speedMs);
        return w;
    }


    struct SetPadding {
        Coord value;
        SetPadding(Coord value): value{value} {}
    };

    template<typename T>
    inline with<T> operator << (with<T> w, SetPadding p) {       
        w->setPadding(p.value);
        return w;
    }

    
} // namespace rckid
