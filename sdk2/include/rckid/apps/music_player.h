#pragma once

#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>

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
                << ResetMenu(mainMenuGenerator /*[]() -> unique_ptr<ui::Menu> { return nullptr; } */);
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                if (carousel_->atRoot()) {
                    // TODO terminate music, etc
                    exit();
                } else {
                    carousel_->moveDown();
                }
            }
            if (btnPressed(Btn::Left))
                carousel_->moveLeft();
            if (btnPressed(Btn::Right))
                carousel_->moveRight();
        }

    private:
        Launcher::BorrowedCarousel * carousel_;


    }; // rckid::MusicPlayer

} // namespace rckid