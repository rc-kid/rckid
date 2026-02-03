#pragma once

#include <functional>

#include <platform.h>

#include <rckid/rckid.h>
#include <rckid/fixedint.h>
#include <rckid/ui/with.h>

namespace rckid::ui {

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

        using OnUpdate = std::function<void(FixedRatio)>;
        using OnDone = std::function<void()>;

        /** Creates new animation. 
         
            The constructor simply registers the animtion in the global list without setting any properties. Use the build functions for that instead.
         */
        Animation() {
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
        virtual ~Animation() {
            // remove from the list
            if (prev_ != nullptr)
                prev_->next_ = next_;
            else
                head_ = next_;
            if (next_ != nullptr)
                next_->prev_ = prev_;
        }

        uint32_t durationMs() const {
            return durationMs_;
        }

        void setDurationMs(uint32_t durationMs) {
            durationMs_ = durationMs;
        }

        OnUpdate const & onUpdate() const { return onUpdate_; }

        void setOnUpdate(OnUpdate onUpdate) {
            onUpdate_ = std::move(onUpdate);
        }

        OnDone const & onDone() const { return onDone_; }

        void setOnDone(OnDone onDone) {
            onDone_ = std::move(onDone);
        }

        Mode mode() const { return mode_; }

        void setMode(Mode mode) {
            mode_ = mode;
        }

        bool active() const { return active_; }

        void start(uint32_t durationMs, Mode mode) {
            mode_ = mode;
            durationMs_ = durationMs;
            start();
        }

        void start() {
            startUs_ = uptimeUs();
            active_ = true;
        }

        void stop() {
            active_ = false;
        }

        void reset() {
            startUs_ = uptimeUs();
        }

        static void updateAll() {
            if (head_ == nullptr)
                return;
            uint32_t currentUs = uptimeUs();
            Animation * x = head_;
            do {
                if (x->active_)
                    x->update(currentUs);
                x = x->next_;
            } while (x != nullptr);
        }

    private:

        void update(uint32_t currentUs) {
            ASSERT(active_ == true);
            uint32_t elapsedMs = (currentUs - startUs_) / 1000;
            uint32_t duration = mode_ == Mode::Oscillate ? durationMs_ * 2 : durationMs_;
            if (elapsedMs >= duration) {
                if (mode_ == Mode::Single) {
                    if (onUpdate_)
                        onUpdate_(FixedRatio::Full());
                    if (onDone_)
                        onDone_();
                    active_ = false;
                    return;
                } else {
                    do {
                        startUs_ += (duration * 1000);
                        elapsedMs -= duration;
                        if (onDone_)
                            onDone_();
                    } while (elapsedMs >= duration);
                }
            }

            if (onUpdate_) {
                if (elapsedMs > durationMs_)
                    elapsedMs = duration - elapsedMs;
                onUpdate_(FixedRatio{elapsedMs, durationMs_});
            }
        }

        OnUpdate onUpdate_;
        OnDone onDone_;

        uint32_t startUs_ = 0;
        uint32_t durationMs_ = 0;
        Mode mode_ = Mode::Single;
        bool active_ = true;

        // next animation in the chain
        Animation * next_ = nullptr;
        Animation * prev_ = nullptr;

        // first animation in the system
        static inline Animation * head_ = nullptr;

    }; // ui::Animation

    struct SetAnimationOnUpdate {
        Animation::OnUpdate onUpdate;
        SetAnimationOnUpdate(Animation::OnUpdate onUpdate): onUpdate{std::move(onUpdate)} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetAnimationOnUpdate sou) {
        w->setOnUpdate(std::move(sou.onUpdate));
        return w;
    }

    struct SetAnimationOnDone {
        Animation::OnDone onDone;
        SetAnimationOnDone(Animation::OnDone onDone): onDone{std::move(onDone)} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetAnimationOnDone sod) {
        w->setOnDone(std::move(sod.onDone));
        return w;
    }

    struct SetAnimationMode {
        Animation::Mode mode;
        SetAnimationMode(Animation::Mode mode): mode{mode} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetAnimationMode sam) {
        w->setMode(sam.mode);
        return w;
    }

    struct SetDurationMs {
        uint32_t durationMs;
        SetDurationMs(uint32_t durationMs): durationMs{durationMs} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetDurationMs sd) {
        w->setDurationMs(sd.durationMs);
        return w;
    }

    struct Start {};
    template<typename T>
    inline with<T> operator << (with<T> w, Start) {
        w->start();
        return w;
    }

} // namespace rckid::ui