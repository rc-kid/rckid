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

        /** Returns true if the task is lightweight. 
         */
        virtual bool lightweight() const { return false; }


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

    protected:
        virtual void onTick() = 0;


    private:
        Task * next_ = nullptr;

        // top of the task stack
        static inline Task * top_ = nullptr;
    }; // rckid::Task

} // namespace rckid