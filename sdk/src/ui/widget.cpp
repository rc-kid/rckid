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

    void Widget::renderEssentials(Rect rect) {
        Animation::updateAll();
        if (Header::shouldRender() && rect.y == 0)
            triggerOnRender(Header::instance());
    }


} // namespace rckid::ui