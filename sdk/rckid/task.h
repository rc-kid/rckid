#pragma once

namespace rckid {
    namespace ui {
        class Header;
    } // namespace ui

    /** Background task base class.
     
        Background tasks can override two functions - tick() and yield() which are called at the respective events - tick every frame and yield every time the system waits & is about to yield for other tasks to run. For normal tasks, tick() is enough, but real-time tasks, such as audio decoding may need to override the yield() method for better latency.
     */
    class Task {
    public:

        /** Simple RAII guard that disables background tasks for its lifetime.
         */
        class DisableGuard {
        public:
            DisableGuard() { Task::disableTasks(); }
            ~DisableGuard() { Task::enableTasks(); }
        };

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
            //if (disable_ > 0)
            //    return;
            Task * x = taskList_;
            while (x != nullptr) {
                x->tick();
                x = x->next_;
            }
        }

        /** Returns true of background tasks are enabled. 
         */
        static bool enabled() { return disable_ == 0; }

        /** Enables background tasks. 
         
            An enable without preceding disable has no effect. Enable should be matched with preceding disable and tasks will only be re-enabled after the call if this was the last disable call to be undone.
         */
        static void enableTasks() {
            if (disable_ > 0)
                disable_--;
        }

        /** Disables background tasks. Each disable has to be matched with an enable, if multiple disable calls are issued, they all have to by undone by their corresponding enable call for the tasks to be enabled again. 
         */
        static void disableTasks() {
            disable_++;
        }

        /** Deletes all tasks that currently run in the background.
         */
        static void killAllTasks() {
            Task * x = taskList_;
            while (x != nullptr) {
                Task * next = x->next_;
                delete x;
                x = next;
            }
            taskList_ = nullptr;
        }

    protected:

        friend class ui::Header;

        Task() {
            next_ = taskList_;
            taskList_ = this;
        }

        virtual void tick() = 0;

        virtual Coord updateHeader([[maybe_unused]] ui::Header & header, Coord endOffset) { return endOffset; }

    private:

        Task * next_ = nullptr;

        static inline Task * taskList_ = nullptr;
        static inline bool yieldActive_ = false;
        static inline uint32_t disable_ = 0;
    };

} // namespace rckid