#pragma once

#include <rckid/app.h>
#include <rckid/memory.h>
#include <rckid/ui/root_widget.h>

namespace rckid::ui {

    template<typename RESULT>
    class App : public ModalApp<RESULT> {
    public:
        explicit App(Rect rect): 
            root_{rect} {
        }

        App(): App{Rect::WH(display::WIDTH, display::HEIGHT)} {}

        Widget * focusedWidget() const { return focusedWidget_; }

        Coord width() const { return root_.width(); }
        Coord height() const { return root_.height(); }


    protected:

        uint32_t animationSpeed() const { return root_.animationSpeed(); }

        void doRefreshStyle() override {
            root_.applyStyle();
            ModalApp<RESULT>::doRefreshStyle();
        }

        void focusWidget(Widget * w) {
            if (w == focusedWidget_)
                return;
            if (focusedWidget_ != nullptr)
                focusedWidget_->onBlur();
            focusedWidget_ = w;
            if (focusedWidget_ != nullptr)
                focusedWidget_->onFocus();
        }

        /** Waits until the given widget becomes idle (all its animations are finished). 
         
            Internally this just runs the render & system tick parts of the application loop without ever going to the loop function. Very useful for synchronous animation events, such as app exit.
         */
        void waitUntilIdle(Widget * w) {
            while (! w->idle()) {
                // ensure we are rendering at proper intervals, i.e. wait for the display update from the previous frame to finish before rendering next
                rckid::display::waitUpdateDone();
                render();
                tick();    
            }
        }

        void waitUntilIdle() { waitUntilIdle(& root_); }

        void onFocus() override {
            ModalApp<RESULT>::onFocus();
            root_.initializeDisplay();
            Header::setVisibility(root_.useHeader());
        }

        /** Renders the widget tree. 
         */
        void loop() override {
            ModalApp<RESULT>::loop();
            if (focusedWidget_ != nullptr)
                focusedWidget_->processEvents();
        }

        void render() override {
            root_.render();
        }

        template<typename T>
        with<T> addChild(T * child) { 
            return root_.addChild(child); 
        }

        Widget::AnimationBuilder animate() { return root_.animate(); }

        void cancelAnimations() { root_.cancelAnimations(); }

        RootWidget root_;

    private:

        Widget * focusedWidget_ = nullptr;

    }; // ui::App<RESULT>

} // namespace rckid::ui