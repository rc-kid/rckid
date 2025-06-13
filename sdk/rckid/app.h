#pragma once

#include <optional>

#include "rckid.h"
#include "utils/stream.h"

namespace rckid {

    /** Base class for RCKid applications.
     
        Defines the application lifecycle and their stacking. 
     
     */
    class App {
    public:
        virtual ~App() = default;

        virtual void save([[maybe_unused]] WriteStream & into) {
            LOG(LL_ERROR, "Saving application state not supported");
        }   

        virtual void load([[maybe_unused]] ReadStream & from) {
            LOG(LL_ERROR, "Loading application state not supported");
        }

        /** Runs given application in its own arena. 
         
            The function takes the optional payload argument to be compatible with main menu action items. 
         */
        template<typename T>
        static void run([[maybe_unused]] void * payload = nullptr) {
            NewArenaGuard g{};
            T * app = new (Arena::alloc<T>()) T{};
            app->loop();
            delete app;
        }

    protected:

        /** Called when the application should gain focus. 
         
            There can only be one focused app at a time. When the app gains focus, it first blurs existing app. If there is existing app, it will become parent of the current app. Then the app resumes own state if suspended and finally sets itself as the focused app. 
         */
        virtual void focus() {
            // if parent is null, then this is new app that is replacing current app (if any). This is called from the run() loop so we should call blur of current app and set our parent to it. Then install ourselves as the current app. 
            if (parent_ == nullptr) {
                parent_ = app_;
                if (app_ != nullptr)
                    app_->blur();
            }
            // otherwise we are the current app and simply should focus ourselves, so continue with setting ourselves as the app and resuming
            app_ = this;
        }

        /** Called when the application should loose focus. 
         
            When loosing focus the app should clear all its resources that can be easily recreated when focused again. 
         */
        virtual void blur() {
            // there are two cases when blur occurs - when we are suspending current ap to launch a new child app, in which case the app_ points to the current app. This happens via focus() of the child and so we just blur ourselves and do nothing. Otherwise if the app is empty, we are closing this app and should terefore focus the parent app 
            if (app_ == nullptr)
                if (parent_ != nullptr)
                    parent_->focus();
        }

        /** */
        virtual void update() {
        }

        /** Method responsible for drawing the app contents on the screen. 
         
            While the method runs, 
         */
        virtual void draw() = 0;

        /** Application main loop. Calling this method executes the application. 
         
            The loop method provides the logic for the periodic updates and drawing of the app. The method is called from the run() method of ModalApp class and should not be called directly.
         */
        virtual void loop();

        /** Exits the app. The app does not exit immediately, but the next time its run method starts a new frame cycle. 
         */
        void exit() { app_ = nullptr;}

    private:

        App * parent_ = nullptr;

        static inline App * app_ = nullptr;

    }; // rckid::App

    template<typename T>
    class ModalApp : public App {
    public:

        std::optional<T> run() {
            loop();
            return result_;
        }

        void exit(T result) {
            result_ = result;
            App::exit();
        }

        using App::exit;

    private:

        std::optional<T> result_;
    }; // ModalApp

    template<>
    class ModalApp<void> : public App {
    public:
        void run() {
            loop();
        }
    }; // ModalApp<void>


    /** Application with Renderer that takes care of its rendering on the display.
     */
    template<typename RENDERER, typename T = void>
    class RenderableApp : public ModalApp<T> {
    protected:

        using ModalApp<T>::exit;
        template <typename... Args>
        RenderableApp(Args&&... args) : g_{std::forward<Args>(args)...} {
        }

        RenderableApp(RenderableApp const &) = delete;
        RenderableApp(RenderableApp &&) = delete;

        void focus() override {
            g_.initialize();
            App::focus();
        }

        void update() override {
            g_.update();
        }

        void draw() override {
            g_.render();
        }

        RENDERER g_;
    }; // rckid::RenderableApp



} // namespace rckid
