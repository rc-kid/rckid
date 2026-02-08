#include <rckid/app.h>

namespace rckid {

    void App::run() {
        if (current_ != nullptr)
            current_->onBlur();
        parent_ = current_;
        current_ = this;
        onFocus();
        while (!shouldExit_) {
            tick();
            loop();
            render();
        }
        onBlur();
        current_ = parent_;
        if (current_ != nullptr)
            current_->onFocus();
        // clear all button events so that the previous app can't react to them any more
        btnClearAll();
    }

} // namespace rckid