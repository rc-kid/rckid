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

        Widget * focusedWidget() const { return focusedWidget_; }

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

        void onFocus() override {
            ModalApp<RESULT>::onFocus();
            root_.initializeDisplay();
        }

        /** Renders the widget tree. 
         */
        void loop() override {
            if (focusedWidget_ != nullptr)
                focusedWidget_->processEvents();
        }

        void render() override {
            root_.render();
        }

        template<typename T>
        with<T> addChild(T * child) { return root_.addChild(child); }

    private:

        RootWidget root_;
        Widget * focusedWidget_ = nullptr;

    }; // ui::App<RESULT>

} // namespace rckid::ui