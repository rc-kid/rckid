#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/carousel.h>

#include <assets/icons_64.h>

namespace rckid {
    
    /** System Home Menu
     
        Home menu that is displeyd by default when the home side button is pressed. The home menu acts as a simple carousel with in app context menu and main device control. Together with the launcher, they form the basic UX of RCKid.

        The home menu utilizes the menu items payload to determine the execution policy, i.e. where & how to execute the action associated with the main menu. The default policy is ExecuteInApp, which means that the home menu will exit and return to the original app the action that was selected so that it can be executed in the context of the appliction itself. 

        The other two execution policies are ExecuteInMenuAndExit, which executes the action immediately in the home menu's context and then exits home menu immediately and the ExecuteInMenu, which executes the action immediately in the home menu's context *and* stays in the home menu. The former is useful for device specific actions (sleep, etc. where the home menu wants to stay in control and ensure proper tear down & up) and the latter is useful for configuration options where more interaction with the home menu is expected after the action (e.g. changing part of app configuration).
    */
    class HomeMenu : public ui::App<ui::MenuItem::ActionEvent> {
    public:

        static constexpr uint32_t ExecuteInApp = 0;
        static constexpr uint32_t ExecuteInMenuAndExit = 1;
        static constexpr uint32_t ExecuteInMenu = 2;

        String name() const override { return "Home menu"; }
        Capabilities capabilities() const override { return {}; }

        static bool active() { return active_; }

        HomeMenu():
            ui::App<ui::MenuItem::ActionEvent>{Rect::XYWH(0, 140, 320, 100), ui::Theme::Accent} 
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
            ui::App<ui::MenuItem::ActionEvent>::onLoopStart();
            ASSERT(parent() != nullptr);
            ui::with(carousel_)
                << ui::ResetMenu([app = parent()] () { return app->homeMenu(); });
        }

        void loop() override {
            ui::App<ui::MenuItem::ActionEvent>::loop();
            if (btnPressed(Btn::Left)) {
                carousel_->moveLeft();
            } else if (btnPressed(Btn::Right)) {
                carousel_->moveRight();
            } else if (btnPressed(Btn::Up)) {
                auto item = carousel_->currentItem();
                if (item->isGenerator()) {
                    carousel_->moveUp(item->generator());
                } else {
                    // execute the action immediately in the menu context
                    item->action()();
                    // and then exit or not based on the payload
                    switch (item->payload) {
                        case ExecuteInApp:
                            exit(std::move(item->action()));
                            break;
                        case ExecuteInMenuAndExit:
                            item->action()();
                            exit();
                            break;
                        case ExecuteInMenu:
                            item->action()();
                            break;
                        default:
                            LOG(LL_ERROR, "Unknown home menu item execution policy " << item->payload);
                    }
                }
            } else if (btnPressed(Btn::Down) || btnPressed(Btn::B)) {
                if (carousel_->atRoot()) {
                    exit();
                } else {
                    carousel_->moveDown();
                }
            }
        }

        void exit(std::optional<ui::MenuItem::ActionEvent> action = std::nullopt) {
            carousel_->set("", ImageSource{}, Direction::Down);
            waitUntilIdle(carousel_);
            if (action.has_value())
                ui::App<ui::MenuItem::ActionEvent>::exit(std::move(action.value()));
            else
                ui::App<ui::MenuItem::ActionEvent>::exit();
        }

    private:

        ui::CarouselMenu * carousel_;

        static inline bool active_ = false;

    }; // HomeMenu

} // namespace rckid