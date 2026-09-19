#pragma once

#include <rckid/ui/app.h>
#include <rckid/apps/utils/file_browser.h>

namespace rckid {

    class FileDialog : public ui::App<String> {
    public:

        String name() const override { return "FileDialog"; }

        FileDialog(String root, fs::Drive drive = fs::Drive::SD):
            drive_{drive},
            root_{std::move(root)} 
        {
            using namespace ui;
            carousel_ = addChild(new ui::CarouselMenu2())
                << SetRect(Rect::XYWH(0, 140, 320, 100));
        }

    protected:

        void onLoopStart() override {
            using namespace ui;
            carousel_->enterMenu([this]() { return FileBrowser::folderMenuGenerator([this](String path){
                exit(std::move(path));
            }, root_, drive_); });
        }

        void onFocus() override {
            ui::App<String>::onFocus();
        }

        void loop() override {
            ui::App<String>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                if (carousel_->menu().parent() == nullptr) {
                    exit();
                    return;
                }
            }
            carousel_->processEvents();
        }

    private:
        fs::Drive drive_;
        String root_;
        ui::CarouselMenu2 * carousel_ = nullptr;
        
    }; // rckid::FileDialog

} // namespace rckid