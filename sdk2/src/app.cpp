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
    }

} // namespace rckid