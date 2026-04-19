#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/progress_bar.h>

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
            ui::App<ui::MenuItem::ActionEvent>{Rect::XYWH(0, 140, 320, 100)} 
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

        void onLoopStart() override;

        void onFocus() override {
            ui::App<ui::MenuItem::ActionEvent>::onFocus();
            focusWidget(carousel_);
        }

        void loop() override {
            ui::App<ui::MenuItem::ActionEvent>::loop();
            if (carousel_->focused()) {
                if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                    auto item = carousel_->currentItem();
                    ASSERT(item != nullptr);
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
                if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                    ASSERT(carousel_->atRoot());
                    exit();
                }
            } else if (progressBar_ != nullptr && progressBar_->idle()) {
                if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                    hideProgressBar();
                if (btnPressed(Btn::Left)) 
                    if (progressBar_->changeValueBy(-1))
                        onProgressBarChange_(progressBar_->value());
                if (btnPressed(Btn::Right)) 
                    if (progressBar_->changeValueBy(1))
                        onProgressBarChange_(progressBar_->value());
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

        /** Shows progress bar in given menu
         */
        void showProgressBar(int32_t min, int32_t max, int32_t value, std::function<void(int32_t)> onChange) {
            ui::Label * label = carousel_->currentLabel();
            progressBar_ = label->addChild(new ui::ProgressBar{})
                << ui::SetRect(Rect::XYWH(0, label->textOffset().y + label->font()->size - 6, label->textWidth(), 20)) 
                << ui::SetRange(min, max)
                << ui::SetValue(value);
            onProgressBarChange_ = onChange;
            progressBar_->animate()
                << ui::FlyIn(progressBar_, Point{0, label->height()})
                << ui::FlyOut(label, Point{0, -18});
            focusWidget(progressBar_);
            waitUntilIdle(progressBar_);
        }

        void hideProgressBar() {
            ASSERT(progressBar_ != nullptr);
            ui::Label * label = carousel_->currentLabel();
            progressBar_->animate()
                << ui::FlyOut(progressBar_, Point{0, label->height()})
                << ui::FlyOut(label, Point{0, 18});
            waitUntilIdle(progressBar_);
            focusWidget(carousel_);
            progressBar_ = nullptr;
            label->clearChildren();
        }

    private:

        ui::CarouselMenu * carousel_;
        ui::ProgressBar * progressBar_ = nullptr;
        std::function<void(int32_t)> onProgressBarChange_;

        static inline bool active_ = false;

    }; // HomeMenu

} // namespace rckid