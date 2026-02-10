#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/carousel.h>

#include <assets/icons_64.h>

namespace rckid {
    
    /** System Home Menu
     
        Home menu that is displeyd by default when the home side button is pressed. The home menu acts as a simple carousel with in app context menu and main device control. Together with the launcher, they form the basic UX of RCKid.

        TODO append the device home menu *after* what we get from user and remember the id - that way we know if we have action that we return, or if the action should be processed directly in the home menu.
     */
    class HomeMenu : public ui::App<void> {
    public:

        String name() const override { return "Home menu"; }
        Capabilities capabilities() const override { return {}; }

        static bool active() { return active_; }

        HomeMenu():
            ui::App<void>{Rect::XYWH(0, 140, 320, 100), ui::Theme::Default} 
        {
            active_ = true;
            root_.useBackgroundImage(false);
            carousel_ = addChild(new ui::CarouselMenu())
                << ui::SetRect(Rect::XYWH(0, 0, 320, 100));
        }

        ~HomeMenu() override {
            active_ = false;
        }

    protected:

        void onLoopStart() override {
            ASSERT(parent() != nullptr);
            ui::with(carousel_)
                << ui::ResetMenu([app = parent()] () { return app->homeMenu(); });
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::Down) || btnPressed(Btn::B)) {
                if (carousel_->atRoot()) {
                    // TODO some pretty animation
                    exit();
                } else {
                    carousel_->moveDown();
                }
            }
        }

    private:
        ui::CarouselMenu * carousel_;

        static inline bool active_ = false;

    }; // HomeMenu

} // namespace rckid