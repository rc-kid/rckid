#pragma once

#include <functional>

#include <platform.h>

#include <rckid/rckid.h>

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

        using OnUpdate = std::function<void(float)>;

        /** Creates new animation. 
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
            return durationUs_ / 1000;
        }

        OnUpdate const & onUpdate() const { return onUpdate_; }

        Animation & setOnUpdate(OnUpdate onUpdate) {
            onUpdate_ = std::move(onUpdate);
            return *this;
        }

        Mode mode() const { return mode_; }

        void setMode(Mode mode) {
            mode_ = mode;
        }

        bool enabled() const { return enabled_; }

        Animation & start(uint32_t durationMs, Mode mode) {
            mode_ = mode;
            startUs_ = uptimeUs();
            durationUs_ = durationMs * 1000;
            enabled_ = true;
            return *this;
        }

        Animation & stop() {
            enabled_ = false;
            return *this;
        }

        Animation & reset() {
            startUs_ = uptimeUs();
            return *this;
        }

        static void updateAll() {
            if (head_ == nullptr)
                return;
            uint32_t currentUs = uptimeUs();
            Animation * x = head_;
            do {
                if (x->enabled_)
                    x->update(currentUs);
                x = x->next_;
            } while (x != nullptr);
        }

    private:

        void update(uint32_t currentUs) {
            ASSERT(enabled_ == true);
            uint32_t elapsedUs = currentUs - startUs_;
            uint32_t duration = mode_ == Mode::Oscillate ? durationUs_ * 2 : durationUs_;
            if (elapsedUs >= duration) {
                if (mode_ == Mode::Single) {
                    if (onUpdate_)
                        onUpdate_(1.0f);
                    enabled_ = false;
                    return;
                } else {
                    do {
                        startUs_ += duration;
                        elapsedUs -= duration;
                    } while (elapsedUs >= duration);
                }
            }

            if (onUpdate_) {
                float f = static_cast<float>(elapsedUs) / static_cast<float>(durationUs_);
                // oscillate mode
                if (f > 1.0f)
                    f = 2.0f - f;
                onUpdate_(f);
            }
        }

        OnUpdate onUpdate_;

        uint32_t startUs_ = 0;
        uint32_t durationUs_ = 0;
        Mode mode_ = Mode::Single;
        bool enabled_ = true;

        // next animation in the chain
        Animation * next_ = nullptr;
        Animation * prev_ = nullptr;

        // first animation in the system
        static inline Animation * head_ = nullptr;

    }; // ui::Animation

} // namespace rckid::ui