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

        App(): root_{Rect::XYWH(0, 0, 320, 240)} {}

        Widget * focusedWidget() const { return focusedWidget_; }

        Coord width() const { return root_.width(); }
        Coord height() const { return root_.height(); }

    protected:

        void focusWidget(Widget * w) {
            if (w == focusedWidget_)
                return;
            if (focusedWidget_ != nullptr)
                focusedWidget_->onBlur();
            focusedWidget_ = w;
            if (focusedWidget_ != nullptr)
                focusedWidget_->onFocus();
        }

        /** Waits until the given widget becomes idle (all its aniations are finished). 
         
            Internally this just runs the render & system tick parts of the application loop without ever going to the loop function. Very useful for synchronous animation events, such as app exit.
         */
        void waitUntilIdle(Widget * w) {
            while (! w->idle()) {
                render();
                tick();    
            }
        }

        void waitUntilIdle() { waitUntilIdle(& root_); }

        void onFocus() override {
            ModalApp<RESULT>::onFocus();
            root_.initializeDisplay();
        }

        /** Renders the widget tree. 
         */
        void loop() override {
            rckid::display::waitUpdateDone();
            if (focusedWidget_ != nullptr)
                focusedWidget_->processEvents();
        }

        void render() override {
            root_.render();
        }

        template<typename T>
        with<T> addChild(T * child) { return root_.addChild(child); }

        Widget::AnimationBuilder animate() { return root_.animate(); }

    private:

        RootWidget root_;
        Widget * focusedWidget_ = nullptr;

    }; // ui::App<RESULT>

} // namespace rckid::ui