#pragma once
#include "../../ui/file_browser.h"

#include "PopupMenu.h"

namespace rckid {

    class FileDialog : public ui::Form<String> {
    public:

        String name() const override { return "FileDialog"; }

        String title() const { return title_; }

        FileDialog(String title, char const * path = "/") : ui::Form<String>{},
            c_{path}, title_{std::move(title)} {
            using namespace ui;
            c_.setRect(Rect::XYWH(0, 160, 320, 80));
            c_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            g_.addChild(c_);
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

    protected:

        void update() override {
            ui::Form<String>::update();
            c_.processEvents();
            // see if an item has been selected, or if we shoudl leave, if up & down were used for traversing the folder strcuture, they have already been cleared by the processEvents
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                LOG(LL_DEBUG, "FileDialog: returning path " << c_.currentPath());
                btnClear(Btn::A);
                btnClear(Btn::Up);
                exit(c_.currentPath());
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
        ui::FileBrowser c_;
        String title_;
        ui::ActionMenu contextMenu_; 

    }; // rckid::FileDialog

} // namespace rckid