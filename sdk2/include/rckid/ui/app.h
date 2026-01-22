#pragma once

#include <rckid/app.h>
#include <rckid/memory.h>
#include <rckid/ui/root_widget.h>

namespace rckid::ui {

// TODO add root widget, that does the rendering and defines the init display & stuff

    template<typename RESULT>
    class App : public ModalApp<RESULT> {
    public:
        App(Rect rect): root_{rect} {}

        App() = default;



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

        template<typename T>
        T * addChild(with<T> const & w) { return root_.addChild(w); }

    private:

        RootWidget root_;

    }; // ui::App<RESULT>

} // namespace rckid::ui