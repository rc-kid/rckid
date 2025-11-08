#pragma once

namespace rckid {

    /** Background task base class.
     
        Background tasks can override two functions - tick() and yield() which are called at the respective events - tick every frame and yield every time the system waits & is about to yield for other tasks to run. For normal tasks, tick() is enough, but real-time tasks, such as audio decoding may need to override the yield() method for better latency.
     */
    class Task {
    public:

        virtual ~Task() {
            // remove the task from the list
            if (taskList_ == this) {
                taskList_ = next_;
            } else {
                Task * x = taskList_;
                while (x != nullptr && x->next_ != this)
                    x = x->next_;
                if (x != nullptr)
                    x->next_ = next_;
            }
        }

        /** Runs the tick method for all registered background tasks.
         */
        static void tickAll() {
            Task * x = taskList_;
            while (x != nullptr) {
                x->tick();
                x = x->next_;
            }
        }

        /** Runs the yield method for all registered background tasks. 

            The yield methods are guarded re-entrance so that if a task's yield method yields itself we won't recurse.
         */
        static void yieldAll() {
            if (yieldActive_)
                return;
            yieldActive_ = true;
            Task * x = taskList_;
            while (x != nullptr) {
                x->yield();
                x = x->next_;
            }
            yieldActive_ = false;
        }

    protected:

        Task() {
            next_ = taskList_;
            taskList_ = this;
        }

        virtual void tick() = 0;

        virtual void yield() {}

    private:

        Task * next_ = nullptr;

        static inline Task * taskList_ = nullptr;
        static inline bool yieldActive_ = false;
    };

} // namespace rckid