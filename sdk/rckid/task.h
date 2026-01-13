#pragma once

#include <functional>

#include "rckid.h"

namespace rckid {
    namespace ui {
        class Header;
    } // namespace ui

    /** Background task base class.
     
        Like apps, background tasks get executed every system tick (frame) via the overriden tick() method. Unlike apps, tasks cannot write to the display, they have no draw() method and must not access any display memory or properties. The only way for a task to visualize its status is via the updateHeader() method that allows the task to add information to the header, if displayed. 

        While tasks essentially allow cooperative multitasking, RCKid is still predominantly single app system as the resources are limited and often running two reasonably complex tasks exceeds the capabilities of the device (a typical example is mp3 playback and https request). Therefore each task must support its destructor being called at any time. 
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
                // this is necessary in case the task deletes itself during tick
                Task * t = x;
                x = x->next_;
                t->tick();
            }
        }

        /** Returns true if there are any active background tasks.
         */
        static bool active() { return taskList_ != nullptr; }

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

        /** Disables background tasks. 
         
            Each disable has to be matched with an enable, if multiple disable calls are issued, they all have to by undone by their corresponding enable call for the tasks to be enabled again. 

            Although the call to disableTasks() kills all existing tasks, the relation between tasks being disabled and new tasks being created is voluntary. The SDK itself will obey the flag and will not schedule new tasks when tasks are disabled, but applications do not have to, which allows them to force single task usage by first disabling tasks and then creating the single task expected to run. 
         */
        static void disableTasks() {
            disable_++;
            if (taskList_ != nullptr)
                killAllTasks();
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

        /** Registers given task constructor for the heartbeat task. 
         
            The task constructor is simply a function that creates the heartbeat task when called and returns it. The heartbeat task is executed every time there is heartbeat interrupt as long as tasks are enabled. The heartbeat task should perform all its actions and when done should delete itself, which also performs task de-registration. 
         */
        static void registerHeartbeatTask(std::function<void()> constructor) {
            heartbeatTaskConstructor_ = constructor;
        }

        /** Runs the heartbeat task if registered and tasks are enabled. Returns false if the task could not be started because tasks are currently disabled, true otherwise.
         */
        static bool runHeartbeatTask() {
            if (disable_ != 0)
                return false;
            if (heartbeatTaskConstructor_ != nullptr)
                heartbeatTaskConstructor_();
            return true;
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
        static inline std::function<void()> heartbeatTaskConstructor_ = nullptr;
    };

} // namespace rckid