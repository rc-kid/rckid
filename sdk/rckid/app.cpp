#include "app.h"

namespace rckid {

    void BaseApp::loop() {
        // if there is already an app, we have to blur & suspend it 
        if (app_ != nullptr) {
            app_->onBlur();
            app_->suspend();
        };
        // set the parent link so that we can go back to the previous app
        parent_ = app_;
        app_ = this;
        app_->onFocus();
        // now run the app
        while (app_ == this) {
            tick();
            update();
            displayWaitUpdateDone();
            draw();
        }
        app_->onBlur();
        // if there was an app, resume & focus
        if (parent_ != nullptr) {
            app_ = parent_;
            app_->resume();
            app_->onFocus();
        }
    }
}