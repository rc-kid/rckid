
#include "app.h"

#include "apps/dialogs/InfoDialog.h"
#include "apps/dialogs/HomeMenu.h"
#include "apps/utils/Alarm.h"
#include "ui/header.h"
#include "task.h"

namespace rckid {

    void App::saveState(String const & name) {
        if (!fs::isMounted())
            return;
        LOG(LL_INFO, "Saving app state " << name);
        String folder = fs::join(homeFolder(), "saves");
        if (!fs::createFolders(folder)) {
            InfoDialog::error("Cannot save state", STR("Cannot create folder " << folder));
            return;
        }
        fs::FileWrite f{fs::fileWrite(fs::join(folder, name))};
        save(f);
    }

    void App::update() {
        // for home menu, first arm when home button is pressed, then show when released. This is to avoid home menu invocation long home button press should turn the device off
        if (! HomeMenu::active()) {
            if (homeMenuArmed_ && btnReleased(Btn::Home)) {
                homeMenuArmed_ = false;
                showHomeMenu();
            } else if (btnPressed(Btn::Home)) {
                homeMenuArmed_ = true;
            }
        }
        if (btnPressed(Btn::VolumeUp)) {
            btnClear(Btn::VolumeUp);
            audioSetVolume(audioVolume() + 1);
        }
        if (btnPressed(Btn::VolumeDown)) {
            btnClear(Btn::VolumeDown);
            uint8_t vol = audioVolume();
            audioSetVolume(vol == 0 ? 0 : vol - 1);
        }
    }

    void App::showHomeMenu() {
        std::optional<ui::Action> a = App::run<HomeMenu>([this]() { return createHomeMenu(); });
        if (a.has_value())
            a.value()();
    }

    ui::ActionMenu * App::createHomeMenu() {
        ui::ActionMenu * menu = new ui::ActionMenu{};
        // application exit 
        menu->add(ui::ActionMenu::Item("Exit", assets::icons_64::logout, [this](){
            exit();
        }));
        // if app supports save states, add save & load actions
        if (supportsSaveState() && fs::isMounted()) {
            menu->add(ui::ActionMenu::Generator("Save state", assets::icons_64::bookmark, [this](){
                ui::ActionMenu * m = new ui::ActionMenu{};
                for (uint32_t i = 1; i <= 4; ++i) {
                    String slotName = STR(i);
                    m->add(ui::ActionMenu::Item(slotName, assets::icons_64::bookmark, [this, i]() {
                        saveState(STR(i));
                    }));
                }
                return m;
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
                    }));
                }
                return m;
            }));
        }
        return menu;
    }

    App::StandaloneModeGuard::StandaloneModeGuard() {
        ui::FormWidget::clearBackgroundImage();
        ui::Header::clearInstance();
        Task::disableTasks();
    }

    App::StandaloneModeGuard::~StandaloneModeGuard() {
        Task::enableTasks();
    }

    extern volatile bool avrStatusRequest_;


    void App::focus() {
        if (parent_ != nullptr)
            parent_->onBlur();
        btnClear();
        // wait for the previous display update to finish to avoid interfering with the old app unloading
        displayWaitUpdateDone();
        // require header refresh when app starts
        ui::Header::requireRefresh();
        // set the current app in focus. If there is previous app, it will be blurred. The focus method also updates the parent app so that we can go back with the apps
        onFocus();
    }

    void App::blur() {
        onBlur();
        btnClear();
        displayWaitUpdateDone();
        // require header refresh when app starts
        ui::Header::requireRefresh();
        if (parent_ != nullptr)
            parent_->onFocus();
    }

    void App::loop() {
        // focus itself (and blur parent)
        focus();
        // now run the app
        while (! shouldExit()) {
            Alarm::checkAlarm();
            update();
            // ticks happen right after update as they request current state from the AVR and this minimizes the likelihood of the arrival of new state during the next update cycle
            displayWaitUpdateDone();
            tick();
            ui::Header::renderIfRequired();
            draw();
            ++redraws_;
            // if we are powering down, exit the app
            
        }
        // blur itself (and focus parent)
        blur();
    }

    void App::secondTick() {
        ui::Header::refresh();
        checkBudget_ = true;
        fps_ = redraws_;
        redraws_ = 0;
        if (app_ != nullptr) {
            app_->onSecondTick();
            // if the app is budgeted and can run, decrease the budget
            if (app_->isBudgeted()) {
                if (app_->verifyBudgetAllowance())
                    budgetSet(budget() - 1);
                else
                    app_->exit();
            } 
        }
    }

    bool App::verifyBudgetAllowance() {
        if (!isBudgeted())
            return true;
        if (budgetProhibitedInterval().contains(timeNow().time)) {
            InfoDialog::error("Cannot be used now", "App usage is not allowed at this time");
            return false;
        }
        uint32_t b = budget();
        if (b == 0) {
            InfoDialog::error("No more budget", "Wait till midnight when budget is reset, or get more");
            return false;
        } else {
            return true;
        }
    }
}