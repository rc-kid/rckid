#pragma once

namespace rckid {

    /** Background task base class.
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

        static void runAll() {
            Task * x = taskList_;
            while (x != nullptr) {
                x->run();
                x = x->next_;
            }
        }

    protected:

        Task() {
            next_ = taskList_;
            taskList_ = this;
        }

        virtual void run() = 0;

    private:

        Task * next_ = nullptr;

        static inline Task * taskList_ = nullptr;
    };

} // namespace rckid