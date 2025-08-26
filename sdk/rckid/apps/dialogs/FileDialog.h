#pragma once
#include "../../ui/file_browser.h"

#include "PopupMenu.h"

namespace rckid {

    class FileDialog : public ui::App<String> {
    public:

        FileDialog() : ui::App<String>{} {
            using namespace ui;
            c_ = g_.addChild(new ui::FileBrowser{"/"});
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            contextMenu_.add(ui::ActionMenu::Item("First"));
            contextMenu_.add(ui::ActionMenu::Item("Second"));
            contextMenu_.add(ui::ActionMenu::Item("And Third"));
            contextMenu_.add(ui::ActionMenu::Item("Some Fourth"));
            contextMenu_.add(ui::ActionMenu::Item("Fantasic Five"));
            contextMenu_.add(ui::ActionMenu::Item("Six"));
            contextMenu_.add(ui::ActionMenu::Item("Seven Se7en"));
            contextMenu_.add(ui::ActionMenu::Item("Eight"));
            contextMenu_.add(ui::ActionMenu::Item("9"));
            contextMenu_.add(ui::ActionMenu::Item("Last But Not Least 10"));
        }

        /** Dialog budgeting mirrors that of its parent.
         */
        bool isBudgeted() const override{ 
            if (parent() != nullptr) 
                return parent()->isBudgeted();
            return true;
        }

    protected:

        void update() override {
            ui::App<String>::update();
            c_->processEvents();
            // see if an item has been selected, or if we shoudl leave, if up & down were used for traversing the folder strcuture, they have already been cleared by the processEvents
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                LOG(LL_DEBUG, "FileDialog: returning path " << c_->currentPath());
                btnClear(Btn::A);
                btnClear(Btn::Up);
                select(c_->currentPath());
            }
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                LOG(LL_DEBUG, "FileDialog: cancelling");
                btnClear(Btn::B);
                btnClear(Btn::Down);
                exit();
            }
            if (btnPressed(Btn::Select)) {
                App::run<PopupMenu<ui::Action>>(& contextMenu_);
            }
        }

    private:
        ui::FileBrowser * c_;
        ui::ActionMenu contextMenu_; 

    }; // rckid::FileDialog

} // namespace rckid