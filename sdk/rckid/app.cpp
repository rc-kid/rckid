
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

    void App::addDefaultHomeActionsInto(ui::ActionMenu * menu) {
        // application exit 
        menu->add(ui::ActionMenu::Item("Exit", assets::icons_64::logout, [this](){
            exit();
        }));
        // if app supports save states, add save & load actions
        if (supportsSaveState()) {
            menu->add(ui::ActionMenu::Item("Save state", assets::icons_64::bookmark, [this](){
                // TODO actually select which slot to use, or let the user choose 
                String saveName = "test";
                saveState(saveName);
                InfoDialog::success("Done", STR("App state saved in " << saveName));
            }));
            menu->add(ui::ActionMenu::Generator("Load state", assets::icons_64::appointment_book, [this](){
                ui::ActionMenu * m = new ui::ActionMenu{};
                fs::Folder folder = fs::folderRead(fs::join(homeFolder(), "saves"));
                for (auto & entry : folder) {
                    if (!entry.isFile())
                        continue;
                    String entryName{entry.name()};
                    m->add(ui::ActionMenu::Item(entryName, assets::icons_64::product, [this, entryName]() {
                        loadState(entryName);
                        InfoDialog::success("Done", STR("App state loaded from " << entryName));
                    }));
                    LOG(LL_DEBUG, "FileBrowser: adding entry " << entry.name());
                }
                return m;
            }));
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
        if (app_ != nullptr)
            app_->verifyBudgetAllowance(true);
        fps_ = redraws_;
        redraws_ = 0;
    }

    bool App::verifyBudgetAllowance(bool decrement) {
        if (!isBudgeted())
            return true;
        uint32_t b = budget();
        if (b == 0) {
            // TODO save to latest slot?
            InfoDialog::error("No more budget", "Wait till midnight when budget is reset, or get more");
            app_->exit();
            return false;
        } else {
            if (decrement)
                budgetSet(b - 1);
            return true;
        }
    }
}