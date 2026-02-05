#pragma once

#include <functional>

#include <platform.h>

#include <rckid/rckid.h>
#include <rckid/fixedint.h>
#include <rckid/ui/with.h>

namespace rckid::ui {

    class Widget;

    /** Animations. 
     
        Animations in the UI are simple objects that can only exist on heap and chain themselves into a global list. The animation system is called be ty the root widget before each frame rendering. 
        
        During the update, the animation's time and other features are updated and if the animation is active, the animation callback method is called to perform the actual animation step.

     */
    class Animation {
    public:

        class Builder;

        /** Animation mode, which can be either a single shot (0...1 and then disable), repeat (0...1 -> 0...1 -> ...), or oscillate (0...1 -> 1...0 -> ...) 
         */
        enum class Mode {
            Single,
            Repeat,
            Oscillate,
        }; 

        using OnUpdate = std::function<void(Widget *, FixedRatio)>;

        /** Creates new animation for given widget.  
         
            The constructor simply registers the animtion in the global list without setting any properties. Use the build functions for that instead.
         */
        Animation(Widget * w);

        Animation(Animation const &) = delete;
        Animation & operator = (Animation const &) = delete;

        /** Deletes the animation and removes it from the global list.
         */
        virtual ~Animation();

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

        Widget * widget() const { return w_; }

        uint32_t durationMs() const { return durationMs_; }

        void setDurationMs(uint32_t durationMs) { durationMs_ = durationMs; }

        Mode mode() const { return mode_; }

        void setMode(Mode mode) { mode_ = mode; }

        OnUpdate const & onUpdate() const { return onUpdate_; }

        void setOnUpdate(OnUpdate onUpdate) { onUpdate_ = std::move(onUpdate); }

        void start() {
            startUs_ = uptimeUs();
            if (onUpdate_)
                onUpdate_(w_, FixedRatio::Empty());
        }

    private:

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
                onUpdate_(w_, FixedRatio{elapsedMs, durationMs_});
            }
            return true;
        }

        Widget * w_ = nullptr;

        OnUpdate onUpdate_;
        
        uint32_t startUs_ = 0;
        uint32_t durationMs_ = 0;
        Mode mode_ = Mode::Single;

        // next animation in the chain
        Animation * next_ = nullptr;
        Animation * prev_ = nullptr;

        // first animation in the system
        static inline Animation * head_ = nullptr;

    }; // ui::Animation

    class Animation::Builder {
    public:
        
        Builder(Widget * widget) {
            a_ = new Animation(widget);
        }

        Builder(Builder & other) = delete;

        Builder(Builder && other) noexcept {
            a_ = other.a_;
            other.a_ = nullptr;
        }

        ~Builder() {
            if (a_ != nullptr)
                a_->start();
        }

        Animation * operator -> () const { return a_; }

    private:
        // no need to delete this as animations are deleted once they are done
        Animation * a_;
    }; 

    struct SetDurationMs {
        uint32_t value;
        SetDurationMs(uint32_t value): value{value} {}
    }; // ui::SetDurationMs
    inline Animation::Builder operator << (Animation::Builder b, SetDurationMs sd) {
        b->setDurationMs(sd.value);
        return b;
    }

    struct SetAnimationMode {
        Animation::Mode mode;
        SetAnimationMode(Animation::Mode mode): mode{mode} {}
    };
    inline Animation::Builder operator << (Animation::Builder b, SetAnimationMode sam) {
        b->setMode(sam.mode);
        return b;
    }

    inline Animation::Builder operator << (Animation::Builder b, Animation::OnUpdate onUpdate) {
        b->setOnUpdate(std::move(onUpdate));
        return b;
    }

} // namespace rckid::ui