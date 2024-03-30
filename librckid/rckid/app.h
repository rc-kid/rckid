#pragma once

#include <vector>

#include "stats.h"

#include "graphics/png.h"
#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    /** Base app class. 

        Apps are always run modally, i.e. the loop is new frame 
    */
    class BaseApp {
    public:

        virtual ~BaseApp() = default;

        /** Runs new app*/
        void run();
        void exit();

    protected:

        virtual void update() = 0;
        virtual void draw() = 0;
        virtual void render() = 0;
        /** Called by the app stack when the app gains focus. 
         */
        virtual void onFocus() {}
        /** Called by the app stack when the app loses focus. 
         */
        virtual void onBlur() {}

    private:

        static inline BaseApp * currentApp_ = nullptr;

    }; // rckid::App


    template<typename DISPLAY_DRIVER> 
    class App : public BaseApp {
    public:
        using Color = typename DISPLAY_DRIVER::Color;

        App(int w = DISPLAY_DRIVER::DEFAULT_WIDTH, int h = DISPLAY_DRIVER::DEFAULT_HEIGHT):
            driver_{w, h} {
        }

    protected:

        void onFocus() override {
            driver_.enable();
        }

        void onBlur() override {
            driver_.disable(); 
        }

        void render() override {
            driver_.render();
        }

        DISPLAY_DRIVER driver_;

    }; // App


} // namespace rckid