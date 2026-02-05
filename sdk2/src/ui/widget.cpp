#include <rckid/ui/widget.h>
#include <rckid/ui/animation.h>

namespace rckid::ui {

    Widget::AnimationBuilder Widget::AnimationBuilder::operator << (Animation * animation) {
        ASSERT(animation != nullptr);
        ASSERT(target_ != nullptr);
        animation->w_ = target_;
        animation->start();
        return *this;
    }

} // namespace rckid::ui