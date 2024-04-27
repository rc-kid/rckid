#pragma once

#include <vector>
#include <optional>

#include "stats.h"
#include "rckid.h"
#include "graphics/png.h"
#include "graphics/primitives.h"
//#include "assets/fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    /** Base app class. 

        Apps are always run modally, i.e. the loop is new frame 
    */
    class BaseApp {
    public:

        virtual ~BaseApp() = default;

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

        /** Main loop. 
         */
        void loop(); 

    private:

        static inline BaseApp * currentApp_ = nullptr;

    }; // rckid::App


    template<typename DISPLAY_DRIVER, typename MODAL_RESULT = void> 
    class App : public BaseApp {
    public:
        using Color = typename DISPLAY_DRIVER::Color;

        App(int w = DISPLAY_DRIVER::DEFAULT_WIDTH, int h = DISPLAY_DRIVER::DEFAULT_HEIGHT):
            driver_{w, h} {
        }
        App(Rect const & rect): driver_{rect} {}

        std::optional<MODAL_RESULT> run() {
            result_ = std::nullopt;
            loop();
            return result_;
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

        using BaseApp::exit;

        void exit(MODAL_RESULT const & result) {
            result_ = result;
            exit();
        }

        DISPLAY_DRIVER driver_;
        std::optional<MODAL_RESULT> result_;

    }; // App

    template<typename DISPLAY_DRIVER>
    class App<DISPLAY_DRIVER, void> : public BaseApp {
    public:
        using Color = typename DISPLAY_DRIVER::Color;

        App(int w = DISPLAY_DRIVER::DEFAULT_WIDTH, int h = DISPLAY_DRIVER::DEFAULT_HEIGHT):
            driver_{w, h} {
        }

        void run() { loop(); }

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
    }; // App<void>


} // namespace rckid