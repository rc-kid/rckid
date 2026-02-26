#pragma once

#include <rckid/error.h>

namespace rckid {

    class Task {
    public:

        /** Creates a new task and adds it to the task stack.
         */
        Task() {
            // add to the task stack
            next_ = top_;
            top_ = this;
        }

        /** Every task must support being killed.
         
            Task destructor simply removes the task from task stack.
         */
        virtual ~Task() {
            // remove from the task stack
            if (top_ == this) {
                top_ = next_;
            } else {
                Task * x = top_;
                while (x != nullptr && x->next_ != this)
                    x = x->next_;
                ASSERT(x != nullptr);
                x->next_ = next_;
            }
        }

        /** Runs the onTick() method for all current tasks.
         
            This is run automatically from rckid's tick() function and does *not* need to be called manually.
         */
        static void runAll() {
            Task * current = top_;
            while (current != nullptr) {
                current->onTick();
                current = current->next_;
            }
        }

        /** Releases resources held by the current tasks.
         */
        static void releaseTaskResources() {
            Task * current = top_;
            while (current != nullptr) {
                // as tasks may also choose to delete themselves as part of releasing the resources we need to get ptr to next first
                Task * x = current;
                current = current->next_;
                x->releaseResources();
            }
        }

    protected:
        virtual void onTick() = 0;

        /** Releases resources held by the task. 
         
            This is analogous to the application's releaseResources except a task may also choose to delete itself as part of releasing the resources. In fact, deleting itself is the preferred form of releasing task resources.
         */
        virtual void releaseResources() { }

    private:
        Task * next_ = nullptr;

        // top of the task stack
        static inline Task * top_ = nullptr;
    }; // rckid::Task

} // namespace rckid