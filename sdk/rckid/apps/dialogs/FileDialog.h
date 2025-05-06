#pragma once
#include "../../ui/file_browser.h"

namespace rckid {

    class FileDialog : public ui::App<String> {
    public:

        FileDialog() : ui::App<String>{} {
            using namespace ui;
            // mount the SD card
            filesystem::mount();
            // TODO whatif not mounted? 
            c_ = new ui::FileBrowser{"/"};
            g_.add(c_);
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
        }

    protected:

        void update() override {
            ui::App<String>::update();
            c_->processEvents();
            // see if an item has been selected
            /*
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                ui::Menu::Item * item = c_->currentItem();
                if (item == nullptr)
                    return;
                exit(item->text());
            }
            */
        }

    private:
        ui::FileBrowser * c_;

    }; // rckid::FileDialog

} // namespace rckid