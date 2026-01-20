#pragma once

#include "../app.h"
#include "../memory.h"
#include "panel.h"

namespace rckid::ui {

// TODO add root widget, that does the rendering and defines the init display & stuff

    template<typename RESULT>
    class App : public ModalApp<RESULT> {
    public:
        App(Rect rect): root_{rect} {}

        App(): root_{Rect::FullScreen()} {}



    protected:

        void onFocus() override {
            ModalApp<RESULT>::onFocus();
            root_.initializeDisplay();
        }

        /** Renders the widget tree. 
         */
        void loop() override {
            root_.render();
        }

        template<typename T>
        T * addChild(T * child) { return root_.addChild(child); }

    private:

        Panel root_;

    }; // ui::App<RESULT>

} // namespace rckid::ui