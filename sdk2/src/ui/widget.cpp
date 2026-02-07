#include <rckid/ui/widget.h>
#include <rckid/ui/animation.h>

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

} // namespace rckid::ui