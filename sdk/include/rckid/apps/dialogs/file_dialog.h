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
            carousel_ = addChild(new Launcher::BorrowedCarousel());
        }

    protected:

        void onLoopStart() override {
            using namespace ui;
            with(carousel_)
                << ResetMenu([this]() { return FileBrowser::folderMenuGenerator([this](String path){
                    exit(std::move(path));
                }, root_, fs::Drive::SD); });
        }

        void onFocus() override {
            ui::App<String>::onFocus();
            focusWidget(carousel_);
        }

        void loop() override {
            ui::App<String>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                ASSERT(carousel_->atRoot());
                // TODO terminate music, etc
                exit();
            }
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                auto item = carousel_->currentItem();
                ASSERT(item->isAction());
                item->action()();
            }
        }

    private:
        fs::Drive drive_;
        String root_;
        Launcher::BorrowedCarousel * carousel_;
        
    }; // rckid::FileDialog

} // namespace rckid