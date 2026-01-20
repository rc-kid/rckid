#pragma once

#include <optional>
#include "rckid.h"

namespace rckid {

    /** RCKid application. 
     
        Application is the code that controls the screen, reacts to the user input, and generally has the command of all of RCKid's resources. The SDK is designed to run a single application at any given time, but applications themselves can stack on top of another, so that the application on top has the control, which will return to the app below when the app exits.
        
        The base class provides common functionality for all apps, such as main loop, basic events and app lifecycle management. Specific apps derive from this class and implement their own extra logic.
     */
    class App {
    public:

        virtual ~App() {
            // TODO assert that we are not current app
        }

        /** Runs the application. 
         */
        void run();

    protected:

        /** Called when the application gains focus. 
         
            Default implementation is empty, and the method must be overriden for extra functionality (with base class onFocus() method called explicitly as well). A typical task for onFocus method is to prepare the screen for drawing, resume audio playback if any, etc.          
         */
        virtual void onFocus() {}

        /** Called when the application loses focus. 
 
            The default implementation is empty. Should be overriden in derived classes for extra functionality. Note that when overriden the base class's onBlur() *must* be called as well.
         */
        virtual void onBlur() {}

        /** Main loop of the application. 
         
            This method is called repeatedly until exit() is called. The method must be overriden in derived classes to provide the actual application logic.
         */
        virtual void loop() = 0;

        /** Flags the application to exit. 
         
            Note that the app does not exit immediately, but rather after the end of its current loop() iteration. When app exists, the control is returned to the parent app, if any.
         */
        void exit() {
            shouldExit_ = true;
        }

        App() {
        }

    private:

        App * parent_ = nullptr;
        // flag indicating whether the app should exit
        bool shouldExit_ = false;

        // currently running app
        static inline App * current_ = nullptr;

    }; // rckid::App

    template<typename T>
    class ModalApp : public App {
    public:

        std::optional<T> run() {
            App::run();
            return std::move(result_);
        }

    protected:
        using App::exit;

        void exit(T && result) {
            result_ = std::move(result);
            exit();
        }

    private:
        std::optional<T> result_;

    }; // rckid::ModalApp<T>

    template<>
    class ModalApp<void> : public App {
    public:
    }; // rckid::ModalApp<void>

} // namespace rckid