#include <rckid/ui/widget.h>
#include <rckid/ui/animation.h>

namespace rckid::ui {

    Animation::Animation(Widget * w):
        w_{w}
    {
        ASSERT(w_ != nullptr);
        ++w_->activeAnimations_;
        // insert into the list
        if (head_ != nullptr) {
            head_->prev_ = this;
        }
        next_ = head_;
        head_ = this;
    }

    Animation::~Animation() {
        ASSERT(w_ != nullptr);
        ASSERT(w_->activeAnimations_ > 0);
        if (--w_->activeAnimations_ == 0)
            w_->onIdle();
        // remove from the list
        if (prev_ != nullptr)
            prev_->next_ = next_;
        else
            head_ = next_;
        if (next_ != nullptr)
            next_->prev_ = prev_;
    }

} // namespace rckid::ui