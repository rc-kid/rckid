#pragma once

namespace rckid {

    class Task {
    public:
        /** Every task must support being killed.  */
        virtual ~Task() = default;

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