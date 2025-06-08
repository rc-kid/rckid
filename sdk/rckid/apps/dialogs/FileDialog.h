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
            c_ = g_.addChild(new ui::FileBrowser{"/"});
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
        }

    protected:

        void update() override {
            ui::App<String>::update();
            c_->processEvents();
            // see if an item has been selected, or if we shoudl leave, if up & down were used for traversing the folder strcuture, they have already been cleared by the processEvents
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                LOG(LL_DEBUG, "FileDialog: returning path " << c_->currentPath());
                exit(c_->currentPath());
            }
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                LOG(LL_DEBUG, "FileDialog: cancelling");
                exit();
            }
        }

    private:
        ui::FileBrowser * c_;

    }; // rckid::FileDialog

} // namespace rckid