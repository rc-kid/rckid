#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    /** Widget that can scroll its contents. 
     
        Useful for showing scrollable contents. The scroll widget simply allows specifying scroll offset, which offsets rendering of all its children by given coordinates. 

        TODO also add optional scrollbars? 
     */
    class ScrollView : public Widget {
    public:

        Point scrollOffset() const { return scrollOffset_; }

        void setScrollOffset(Point value) { 
            scrollOffset_ = value;
        }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            column += scrollOffset_.x;
            startRow += scrollOffset_.y;
            renderChildren(column, startRow, buffer, numPixels);
        }

        void applyStyle(Style const & style) override {
            Widget::applyStyle(style);
        }

    protected:

        Point scrollOffset_{0,0};

    }; // ui::ScrollView

    /** Animation template for scrolling the view to given coordinates from the current ones. 
     */
    inline Animation * ScrollTo(ScrollView * target, Point to) {
        return (new Animation{
            [from = target->scrollOffset(), to, target](FixedRatio progress) {
                Coord x = from.x + progress.scale(to.x - from.x);
                Coord y = from.y + progress.scale(to.y - from.y);
                target->setScrollOffset(Point{x, y});
            },
            target->animationSpeed()
        })->setEasingFunction(easing::inOut);
    }

} // namespace rckid::ui