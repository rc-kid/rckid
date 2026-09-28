#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/ui/widget.h>
#include <rckid/ui/animation.h>
#include <rckid/ui/header.h>

namespace rckid::ui {

    void Widget::cancelAnimations() {
        if (activeAnimations_ > 0)
            Animation::cancelAnimationsFor(this);
        ASSERT(activeAnimations_ == 0);
    }

    Widget::AnimationBuilder Widget::AnimationBuilder::operator << (Animation * animation) {
        ASSERT(animation != nullptr);
        ASSERT(target_ != nullptr);
        animation->w_ = target_;
        animation->start();
        return *this;
    }

    void Widget::animateIn(Point distance, InOutDirection dir) {
        switch (dir) {
            case InOutDirection::None:
                break;
            case InOutDirection::Top: {
                Coord maxY = 0;
                for (auto & child : children_)
                    if (child->y() > maxY)
                        maxY = child->y();
                maxY = std::min(maxY, height());
                for (auto & child : children_)
                    if (child->visibleInParent())
                        animate() << FlyIn(child.get(), distance)->setDelayMs(maxY - child->y());
                break;
            }
            case InOutDirection::LeftRight: {
                UNIMPLEMENTED;
                break;
            }
        }
    }

    void Widget::animateOut(Point distance, InOutDirection dir) {
        switch (dir) {
            case InOutDirection::None:
                break;
            case InOutDirection::Top: {
                Coord minY = height();
                for (auto & child : children_)
                    if (child->y() < minY)
                        minY = child->y();
                minY = std::max<Coord>(minY, 0);
                for (auto & child : children_)
                    if (child->visibleInParent())
                        animate() << FlyOut(child.get(), distance)->setDelayMs(child->y() - minY);
                break;
            }
            case InOutDirection::LeftRight: {
                UNIMPLEMENTED;
                break;
            }
        }
    }

    void Widget::renderOn(Canvas & canvas, Point where) {
        if (! visible())
            return;
        // reset any per frame rendering state of the widget and its children (the Label's render hint)
        triggerOnRender(this);
        // clip the widget's rows against the canvas. startRow is relative to the widget's top, the buffer is offset by where the widget starts in the canvas column
        Coord startRow = (where.y < 0) ? -where.y : 0;
        Coord bufferOffset = (where.y < 0) ? 0 : where.y;
        Coord numPixels = std::min(height() - startRow, canvas.height() - bufferOffset);
        if (numPixels <= 0)
            return;
        // render right to left, which is both the native refresh direction and what the widgets with per frame rendering state expect
        for (Coord column = width() - 1; column >= 0; --column) {
            Coord x = where.x + column;
            if (x < 0)
                break; // no column to our left can be visible either
            if (x >= canvas.width())
                continue;
            renderColumn(column, startRow, canvas.columnBuffer(x) + bufferOffset, numPixels);
        }
    }

    void Widget::renderEssentials(Rect rect) {
        Animation::updateAll();
        if (Header::shouldRender() && rect.y == 0)
            triggerOnRender(Header::instance());
    }


} // namespace rckid::ui