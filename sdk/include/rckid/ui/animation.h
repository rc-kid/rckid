#pragma once

#include <functional>

#include <platform.h>

#include <rckid/rckid.h>
#include <rckid/fixedint.h>
#include <rckid/ui/with.h>
#include <rckid/ui/widget.h>

namespace rckid::ui {

    namespace easing {

        /** Identity easing function (sentinel value).
         */
        inline FixedRatio identity(FixedRatio progress) { return progress; }

        /** Converts the progress from 0..1 into values from 0..1..0 (i.e. forth and back).
         */
        inline FixedRatio mirror(FixedRatio progress) {
            if (progress < FixedRatio{0.5f})
                return progress * 2;
            else
                return (FixedRatio::Full() - progress) * 2;
        }

        /** Quadratic ease in (starts slow, then goes fast).
         */
        inline FixedRatio in(FixedRatio progress) {
            return progress * progress;
        }

        /** Quadratic ease out (starts fast, then goes slow).
         */
        inline FixedRatio out(FixedRatio progress) {
            FixedRatio inv{FixedRatio::Full() - progress};
            return FixedRatio::Full() - inv * inv;
        }

        /** Quadratic ease in-out (starts slow, then goes fast, then goes slow again).
         */
        inline FixedRatio inOut(FixedRatio progress) {
            if (progress < FixedRatio{0.5f}) {
                return progress * progress * 2;
            } else {
                FixedRatio inv{FixedRatio::Full() - progress};
                return FixedRatio::Full() - inv * inv * 2;
            }
        }

        /** Applies mirror and inOut transformation that first accelerates from 0 decelerating towards 1, then accelerates from 1 decelerating towards 0. */
        inline FixedRatio inOutIn(FixedRatio progress) {
            return inOut(mirror(progress));
        }

    } // rckid::ui::easing

    /** Animations. 
     
        Animations in the UI are simple objects that can only exist on heap and chain themselves into a global list. The animation system is called be ty the root widget before each frame rendering. 
        
        During the update, the animation's time and other features are updated and if the animation is active, the animation callback method is called to perform the actual animation step.

     */
    class Animation {
    public:

        /** Animation update callback type. 
         
            The callback is std::function so that capture can be used and takes the target widget and the animation progress ratio (between 0 and 1) as parameters. It is guaranteed that the callback is first called with ratio 0 when the animation stars and the last call of the callback function is with progress equal to 1.
         */
        using OnUpdate = std::function<void(FixedRatio)>;

        /** Easing function type. 
         
            This is just a a simple function that takes the progress ratio and adjusts it to a different ratio. As the easing functions are intended to be simple value transformers, function pointer instead of std::function is used to avoid unnecessary overhead.
         */
        typedef FixedRatio (*EasingFunction)(FixedRatio);

        /** Creates new animation for given widget.  
         
            The constructor simply registers the animtion in the global list without setting any properties. Use the build functions for that instead.
         */
        Animation(OnUpdate onUpdate, uint32_t durationMs):
            onUpdate_{std::move(onUpdate)},
            durationMs_{durationMs}
        {
            // insert into the list
            if (head_ != nullptr) {
                head_->prev_ = this;
            }
            next_ = head_;
            head_ = this;
        }

        Animation(Animation const &) = delete;
        Animation & operator = (Animation const &) = delete;

        /** Deletes the animation and removes it from the global list.
         */
        ~Animation() {
            if (w_ != nullptr) {
                ASSERT(w_->activeAnimations_ > 0);
                if (--w_->activeAnimations_ == 0)
                    w_->onIdle();
            }
            // remove from the list
            if (prev_ != nullptr)
                prev_->next_ = next_;
            else
                head_ = next_;
            if (next_ != nullptr)
                next_->prev_ = prev_;
        }

        static void updateAll() {
            if (head_ == nullptr)
                return;
            uint32_t currentUs = time::uptimeUs();
            Animation * x = head_;
            do {
                if (! x->update(currentUs)) {
                    Animation * toDelete = x;
                    x = x->next_;
                    delete toDelete;
                } else {
                    x = x->next_;
                }
            } while (x != nullptr);
        }

        static void cancelAnimationsFor(Widget * w) {
            Animation * x = head_;
            while (x != nullptr) {
                if (x->w_ == w) {
                    Animation * toDelete = x;
                    x = x->next_;
                    delete toDelete;
                } else {
                    x = x->next_;
                }
            }
        }

        Widget * widget() const { return w_; }

        uint32_t durationMs() const { return durationMs_; }

        Animation * setDurationMs(uint32_t durationMs) { 
            durationMs_ = durationMs; 
            return this;
        }

        uint32_t delayMs() const { return delayMs_; }

        Animation * setDelayMs(uint32_t delayMs) { 
            delayMs_ = delayMs; 
            return this;
        }

        bool repeat() const { return repeat_;}

        Animation * setRepeat(bool value = true) { 
            repeat_ = value; 
            return this;
        }

        OnUpdate const & onUpdate() const { return onUpdate_; }

        EasingFunction easingFunction() const { return easingFunction_; }

        Animation * setEasingFunction(EasingFunction easingFunction) { 
            easingFunction_ = easingFunction; 
            return this;
        }

    private:

        friend class Widget::AnimationBuilder;

        void start() {
            startUs_ = time::uptimeUs();
            if (w_ != nullptr)
                ++w_->activeAnimations_;
            if (onUpdate_)
                onUpdate_(FixedRatio::Empty());
        }

        bool update(uint32_t currentUs) {
            uint32_t elapsedMs = (currentUs - startUs_) / 1000;
            if (elapsedMs < delayMs_)
                return true;
            elapsedMs -= delayMs_;
            if (elapsedMs >= durationMs_) {
                if (repeat_) {
                    do {
                        startUs_ += (durationMs_ * 1000);
                        elapsedMs -= durationMs_;
                    } while (elapsedMs >= durationMs_);
                } else {
                    if (onUpdate_)
                        onUpdate_(easingFunction_(FixedRatio::Full()));
                    return false;
                }
            }
            if (onUpdate_) {
                if (elapsedMs > durationMs_)
                    elapsedMs = durationMs_ - elapsedMs;
                onUpdate_(easingFunction_(FixedRatio{elapsedMs, durationMs_}));
            }
            return true;
        }

        Widget * w_ = nullptr;

        OnUpdate onUpdate_;
        
        uint32_t startUs_ = 0;
        uint32_t delayMs_ = 0;
        uint32_t durationMs_ = 0;
        bool repeat_ = false;
        EasingFunction easingFunction_ = easing::identity;

        // next animation in the chain
        Animation * next_ = nullptr;
        Animation * prev_ = nullptr;

        // first animation in the system
        static inline Animation * head_ = nullptr;

    }; // ui::Animation

    inline Animation * Move(Widget * target, Point from, Point to) {
        return (new Animation{
            [from, to, target](FixedRatio progress) {
                Coord x = from.x + progress.scale(to.x - from.x);
                Coord y = from.y + progress.scale(to.y - from.y);
                target->setRect(Rect::XYWH(x, y, target->width(), target->height()));
            },
            target->animationSpeed()
        })->setEasingFunction(easing::inOut);
    }

    inline Animation * MoveTo(Widget * target, Point to) {
        return Move(target, target->position(), to);
    }

    inline Animation * MoveAndResize(Widget * target, Rect to) {
        return (new Animation{
            [from = target->rect(), to, target](FixedRatio progress) {
                Coord x = from.x + progress.scale(to.x - from.x);
                Coord y = from.y + progress.scale(to.y - from.y);
                Coord width = from.width() + progress.scale(to.width() - from.width());
                Coord height = from.height() + progress.scale(to.height() - from.height());
                target->setRect(Rect::XYWH(x, y, width, height));
            },
            target->animationSpeed()
        })->setEasingFunction(easing::inOut);
    }

    inline Animation * MoveHorizontally(Widget * target, Coord fromX, Coord toX) {
        return (new Animation{
            [fromX, toX, target](FixedRatio progress) {
                Coord x = fromX + progress.scale(toX - fromX);
                target->setRect(Rect::XYWH(x, target->rect().y, target->width(), target->height()));
            },
            target->animationSpeed()
        })->setEasingFunction(easing::inOut);
    }

    inline Animation * MoveVertically(Widget * target, Coord fromY, Coord toY) {
        return (new Animation{
            [fromY, toY, target](FixedRatio progress) {
                Coord y = fromY + progress.scale(toY - fromY);
                target->setRect(Rect::XYWH(target->rect().x, y, target->width(), target->height()));
            },
            target->animationSpeed()
        })->setEasingFunction(easing::inOut);
    }

    inline Animation * FlyIn(Widget * target, Point distance = Point{0, -240}) {
        return Move(target, target->position() + distance, target->position());
    }

    inline Animation *FlyOut(Widget * target, Point distance = Point{0, -240}) {
        return Move(target, target->position(), target->position() + distance);
    }
    // easing curves


} // namespace rckid::ui