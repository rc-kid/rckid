
#include "app.h"

#include "apps/dialogs/InfoDialog.h"
#include "apps/dialogs/HomeMenu.h"
#include "ui/header.h"



namespace rckid {

    void App::update() {
        if (btnPressed(Btn::Home)) {
            std::optional<ui::Action> a = App::run<HomeMenu>(homeMenuGenerator());
            if (a.has_value())
                a.value()();
        }
    }

    void App::loop() {
        // wait for the previous display update to finish to avoid interfering with the old app unloading
        displayWaitUpdateDone();
        // set the current app in focus. If there is previous app, it will be blurred. The focus method also updates the parent app so that we can go back with the apps
        focus();
        // now run the app
        while (app_ == this) {
            tick();
            update();
            displayWaitUpdateDone();
            draw();
            ++redraws_;
        }
        // wait for the last display update to finish so that the display routine does not interfere with the app unloading
        displayWaitUpdateDone();
        // we are done, should blur ourselves, and refocus parent (if any)
        blur();
    }

    void App::onSecondTick() {
        ui::Header::refresh();
        if (app_ != nullptr && app_->verifyBudgetAllowance()) {
            InfoDialog::error("No more budget", "Wait till midnight when budget is reset, or get more");
            app_->exit();
        }
        fps_ = redraws_;
        redraws_ = 0;
    }
}