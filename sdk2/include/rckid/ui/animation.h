#pragma once

#include <functional>

#include <platform.h>

#include <rckid/rckid.h>
#include <rckid/fixedint.h>
#include <rckid/ui/with.h>
#include <rckid/ui/widget.h>

namespace rckid::ui {

    namespace easing {

        inline FixedRatio identity(FixedRatio progress) { return progress; }

        inline FixedRatio in(FixedRatio progress) {
            return progress * progress;
        }

        inline FixedRatio out(FixedRatio progress) {
            FixedRatio inv{FixedRatio::Full() - progress};
            return FixedRatio::Full() - inv * inv;
        }

        inline FixedRatio inOut(FixedRatio progress) {
            if (progress < FixedRatio{0.5f}) {
                return progress * progress * 2;
            } else {
                FixedRatio inv{FixedRatio::Full() - progress};
                return FixedRatio::Full() - inv * inv * 2;
            }
        }

    } // rckid::ui::easing

    /** Animations. 
     
        Animations in the UI are simple objects that can only exist on heap and chain themselves into a global list. The animation system is called be ty the root widget before each frame rendering. 
        
        During the update, the animation's time and other features are updated and if the animation is active, the animation callback method is called to perform the actual animation step.

     */
    class Animation {
    public:

        /** Animation mode, which can be either a single shot (0...1 and then disable), repeat (0...1 -> 0...1 -> ...), or oscillate (0...1 -> 1...0 -> ...) 
         */
        enum class Mode {
            Single,
            Repeat,
            Oscillate,
        }; 

        using OnUpdate = std::function<void(Widget *, FixedRatio)>;

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
            uint32_t currentUs = uptimeUs();
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

        Mode mode() const { return mode_; }

        Animation * setMode(Mode mode) { 
            mode_ = mode; 
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
            startUs_ = uptimeUs();
            if (w_ != nullptr)
                ++w_->activeAnimations_;
            if (onUpdate_)
                onUpdate_(w_, FixedRatio::Empty());
        }

        bool update(uint32_t currentUs) {
            uint32_t elapsedMs = (currentUs - startUs_) / 1000;
            uint32_t duration = mode_ == Mode::Oscillate ? durationMs_ * 2 : durationMs_;
            if (elapsedMs >= duration) {
                if (mode_ == Mode::Single) {
                    if (onUpdate_)
                        onUpdate_(w_, FixedRatio::Full());
                    return false;
                } else {
                    do {
                        startUs_ += (duration * 1000);
                        elapsedMs -= duration;
                    } while (elapsedMs >= duration);
                }
            }

            if (onUpdate_) {
                if (elapsedMs > durationMs_)
                    elapsedMs = duration - elapsedMs;
                onUpdate_(w_, easingFunction_(FixedRatio{elapsedMs, durationMs_}));
            }
            return true;
        }

        Widget * w_ = nullptr;

        OnUpdate onUpdate_;
        
        uint32_t startUs_ = 0;
        uint32_t durationMs_ = 0;
        Mode mode_ = Mode::Single;
        EasingFunction easingFunction_ = easing::identity;

        // next animation in the chain
        Animation * next_ = nullptr;
        Animation * prev_ = nullptr;

        // first animation in the system
        static inline Animation * head_ = nullptr;

    }; // ui::Animation

    inline Animation * Move(Point from, Point to, uint32_t durationMs) {
        return (new Animation{
            [from, to](Widget * w, FixedRatio progress) {
                Coord x = from.x + progress.scale(to.x - from.x);
                Coord y = from.y + progress.scale(to.y - from.y);
                w->setRect(Rect::XYWH(x, y, w->width(), w->height()));
            },
            durationMs
        })->setEasingFunction(easing::inOut);
    }

    inline Animation * Move(Widget * target, Point from, Point to, uint32_t durationMs) {
        return (new Animation{
            [from, to, target](Widget *, FixedRatio progress) {
                Coord x = from.x + progress.scale(to.x - from.x);
                Coord y = from.y + progress.scale(to.y - from.y);
                target->setRect(Rect::XYWH(x, y, target->width(), target->height()));
            },
            durationMs
        })->setEasingFunction(easing::inOut);
    }

    inline Animation * MoveHorizontally(Coord fromX, Coord toX, uint32_t durationMs) {
        return (new Animation{
            [fromX, toX](Widget * w, FixedRatio progress) {
                Coord x = fromX + progress.scale(toX - fromX);
                w->setRect(Rect::XYWH(x, w->rect().y, w->width(), w->height()));
            },
            durationMs
        })->setEasingFunction(easing::inOut);
    }

    inline Animation * MoveHorizontally(Widget * target, Coord fromX, Coord toX, uint32_t durationMs) {
        return (new Animation{
            [fromX, toX, target](Widget *, FixedRatio progress) {
                Coord x = fromX + progress.scale(toX - fromX);
                target->setRect(Rect::XYWH(x, target->rect().y, target->width(), target->height()));
            },
            durationMs
        })->setEasingFunction(easing::inOut);
    }

    inline Animation * MoveVertically(Coord fromY, Coord toY, uint32_t durationMs) {
        return (new Animation{
            [fromY, toY](Widget * w, FixedRatio progress) {
                Coord y = fromY + progress.scale(toY - fromY);
                w->setRect(Rect::XYWH(w->rect().x, y, w->width(), w->height()));
            },
            durationMs
        })->setEasingFunction(easing::inOut);
    }

    inline Animation * MoveVertically(Widget * target, Coord fromY, Coord toY, uint32_t durationMs) {
        return (new Animation{
            [fromY, toY, target](Widget *, FixedRatio progress) {
                Coord y = fromY + progress.scale(toY - fromY);
                target->setRect(Rect::XYWH(target->rect().x, y, target->width(), target->height()));
            },
            durationMs
        })->setEasingFunction(easing::inOut);
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

    // easing curves


} // namespace rckid::ui