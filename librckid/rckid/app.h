#pragma once

#include <vector>

#include "stats.h"

#include "graphics/png.h"
#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    /** Base app class. 

        Apps are always run modally, i.e. the loop is new frame 
    */
    class App {
    public:

        virtual ~App() = default;

        /** Runs new app*/
        void run();
        void exit();

    protected:

        virtual void update() = 0;
        virtual void draw() = 0;
        virtual void render() = 0;
        /** Called by the app stack when the app gains focus. 
         */
        virtual void onFocus() = 0;
        /** Called by the app stack when the app loses focus. 
         */
        virtual void onBlur() = 0;

    private:

        static inline App * currentApp_ = nullptr;

    }; // rckid::App

} // namespace rckid