#pragma once

#include "rckid.h"
#include "utils/stream.h"

namespace rckid {

    /** Base class for RCKid applications.
     
        Defines the application lifecycle and their stacking. 
     
     */
    class BaseApp {
    public:

        /** Runs the application. 
         */
        void loop(); 

        /** Suspends the app. 
         
            For apps that support suspending, the method should be overriden and the app should free as much memory as possible. 
         */
        virtual void suspend() {
            ASSERT(suspended_ == false);
            suspended_ = true;
        }

        virtual void resume() {
            ASSERT(suspended_ == true);
            suspended_ = false;
        }

        virtual void save([[maybe_unused]] WriteStream & into) {
            LOG(LL_ERROR, "Saving application state not supported");
        }   

        virtual void load([[maybe_unused]] ReadStream & from) {
            LOG(LL_ERROR, "Loading application state not supported");
        }

    protected:

        /** Called when the application gains focus, 
         
            i.e. its update & draw methods start being called. This is either after the app is created and before it starts running, or if it was superseded by another app, when that app exitted.  
         */
        virtual void onFocus() {}

        /** Called when the application is about to loose focus.
         
            i.e. no upate or draw methods will be called after the application blurs. This can happen when either the app exits, or if it is superseded by another app. 
         */
        virtual void onBlur() {}

        /** */
        virtual void update() {

        }

        /** Method responsible for drawing the app contents on the screen. 
         
            While the method runs, 
         */
        virtual void draw() = 0;

    private:

        bool suspended_ = false;
        BaseApp * parent_ = nullptr;

        static inline BaseApp * app_ = nullptr;

    }; // rckid::BaseApp

    /** Application with Renderer that takes care of its rendering on the display.
     */
    template<typename RENDERER>
    class App : public BaseApp {
    public:
        using Color = typename RENDERER::Color;
    protected:
        template <typename... Args>
        App(Args&&... args) : g_{std::forward<Args>(args)...} {
        }

        void onFocus() override {
            g_.initialize();
        }

        void draw() override {
            g_.render();
        }

        RENDERER g_;

    }; // rckid::App

} // namespace rckid
