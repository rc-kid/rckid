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

    void Widget::flyIn(Point distance) {
        Coord maxY = 0;
        for (auto & child : children_)
            if (child->y() > maxY)
                maxY = child->y();
        maxY = std::min(maxY, height());
        for (auto & child : children_)
            if (child->visibleInParent())
                animate() << FlyIn(child.get(), distance)->setDelayMs(maxY - child->y());
    }

    void Widget::flyOut(Point distance) {
        Coord minY = height();
        for (auto & child : children_)
            if (child->y() < minY)
                minY = child->y();
        minY = std::min<Coord>(minY, 0);
        for (auto & child : children_)
            if (child->visibleInParent())
                animate() << FlyOut(child.get(), distance)->setDelayMs(child->y()-minY);
    }

    void Widget::renderEssentials() {
        Animation::updateAll();
        if (Header::shouldRender())
            triggerOnRender(Header::instance());
    }


} // namespace rckid::ui