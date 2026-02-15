#pragma once

#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/file_browser.h>

#include <rckid/audio/mp3.h>

namespace rckid {

    /** Simple music player. 
     */
    class MusicPlayer : public ui::App<void> {
    public:

        String name() const override { return "Music"; }

        MusicPlayer() {
            using namespace ui;
            carousel_ = addChild(new Launcher::BorrowedCarousel());
        }

    protected:

        void onLoopStart() override {
            using namespace ui;
            with(carousel_)
                << ResetMenu([]() { return FileBrowser::folderMenuGenerator(nullptr, "/files/music", fs::Drive::SD); });
        }

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(carousel_);
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                ASSERT(carousel_->atRoot());
                // TODO terminate music, etc
                exit();
            }
        }

    private:
        Launcher::BorrowedCarousel * carousel_;


    }; // rckid::MusicPlayer

} // namespace rckid