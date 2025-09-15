#pragma once

#include "../MainMenu.h"
#include "../../ui/carousel.h"
#include "../../ui/menu.h"
#include "../../assets/icons_64.h"
#include "../../assets/fonts/OpenDyslexic64.h"
#include "Slider.h"


namespace rckid {

    /** Home menu dialog.
     
        The home menu is always accessible by short press of the home button and allows setting the basic functionality such as powering the device off, setting it to sleep, controlling the radios, brightness and sound volume, etc. 
        
        Furthermore, each application can add its own menu items that can deal with things like saving & restoring the app previous state, etc. As a modal app, the result of running home menu is this custom app action that the app needs to execute (this is to ensure the action is executed in correct app context). 

        General home menu actions (settings, modes, etc.) are executed as part of the home menu and do not always return to the menu. 
     */
    class HomeMenu : public ui::Form<ui::Action> {
    public:

        String name() const override { return "HomeMenu"; }

        HomeMenu() : HomeMenu{nullptr} {}

        HomeMenu(ui::ActionMenu::MenuGenerator generator):
            ui::Form<ui::Action>{Rect::XYWH(0, 144, 320, 96), /* raw */ true} {
            g_.addChild(c_);
            c_.setRect(Rect::XYWH(0, 0, 320, 96));
            c_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            customGenerator_ = std::move(generator);
            active_ = true;
        }

        ~HomeMenu() {
            active_ = false;
        }

        /** Home menu is explicitly not budgeted app.
         */
        bool isBudgeted() const override { return false; }

        /** Returns if there is home menu active in the system (i.e. if home menu is anywhere on the app stack). 
         
            If this function returns true, current app runs as part of home menu.
         */
        static bool active() { return active_; }

    protected:

        void focus() override {
            ui::Form<ui::Action>::focus();
            if (c_.menu() == nullptr) {
                c_.setMenu([this](){
                    ui::ActionMenu * m = customGenerator_ == nullptr ? (new ui::ActionMenu{}) : customGenerator_();;
                    extendMenu(m);
                    return m;
                });
            }
        }

        void update() override {
            g_.update();
            if (! c_.processEvents()) {
                if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                    btnClear(Btn::B);
                    btnClear(Btn::Down);
                    exit();
                };
                if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                    auto action = c_.currentItem();
                    ASSERT(action->isAction());
                    if (isCurrentItemCustom())
                        exit(action->action());
                    else 
                        action->action()();
                };
            }
        }

    private:

        bool isCurrentItemCustom() {
            auto history = c_.history();
            if (history == nullptr)
                return c_.currentIndex() < customItems_;
            while (history->previous != nullptr)
                history = history->previous;
            return history->index < customItems_;
        }

        void extendMenu(ui::ActionMenu * menu) {
            // set custom items to after exit (exit returns to the previous app as well)
            customItems_ = menu->size();
            menu->add(ui::ActionMenu::Item("Volume", assets::icons_64::high_volume, [this](){
                Slider s{assets::icons_64::high_volume, "Volume", 0, 15, audioVolume(), [](uint32_t value) { 
                    audioSetVolume(value); 
                }};
                s.setAnimation(c_.iconPosition(), c_.textPosition());
                s.loop();
            }));
            menu->add(ui::ActionMenu::Item("Brightness", assets::icons_64::brightness, [this](){
                Slider s{assets::icons_64::brightness, "Brightness", 0, 15, displayBrightness() >> 4, [](uint32_t value) {
                     displaySetBrightness(value * 16 + value); 
                }};
                s.setAnimation(c_.iconPosition(), c_.textPosition());
                s.loop();
            }));
            menu->add(ui::ActionMenu::Item("Plane mode", assets::icons_64::airplane_mode, [](){
                InfoDialog::info("Plane mode", "Plane mode selected");
                // TODO airplane mode
            }));
            menu->add(ui::ActionMenu::Item("Sleep", assets::icons_64::sleeping, []() {
                // TODO exit & sleep
            }));
            menu->add(ui::ActionMenu::Item("Power Off", assets::icons_64::power_off, [](){
                // TODO power off
            }));
        }

        ui::CarouselMenu<ui::Action> c_;

        // custom generator supplied by the app
        ui::ActionMenu::MenuGenerator customGenerator_; 
        // number of custom actions
        uint32_t customItems_ = 0;
        // determines whether the home menu is inside custom items returned by the app (i.e. will return to the app), or the home menu items which return to the home menu by default.
        bool activeCustomItem_ = false;

        // true if there is home menu active in the system
        static inline bool active_ = false;

    }; // rckid::HomeMenu

} // namespace rckid