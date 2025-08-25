#pragma once

#include "../MainMenu.h"
#include "../../ui/carousel.h"
#include "../../ui/menu.h"
#include "../../assets/icons_64.h"
#include "../../assets/fonts/OpenDyslexic64.h"


namespace rckid {

    /** Home menu dialog.
     
        The home menu is always accessible by short press of the home button and allows setting the basic functionality such as powering the device off, setting it to sleep, controlling the radios, brightness and sound volume, etc. Furthermore, each application can add its own menu items that can deal with things like saving & restoring the app previous state, etc. 
     */
    class HomeMenu : public ui::App<bool> {
    public:

        HomeMenu() : HomeMenu{new ui::Menu{}} {}

        HomeMenu(ui::Menu * menu):
            ui::App<bool>{Rect::XYWH(0, 160, 320, 80), /* raw */ true} {
            g_.addChild(c_);
            c_.setRect(Rect::XYWH(0, 0, 320, 80));
            c_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            // append basic functions to the menu
            menu->add(MainMenu::Action("Exit", assets::icons_64::poo, [this](){
                exit(true);
            }));
            menu->add(MainMenu::Action("Plane mode", assets::icons_64::airplane_mode, [](){
                // TODO airplane mode
            }));
            menu->add(MainMenu::Action("Sleep", assets::icons_64::poo, []() {
                // TODO exit & sleep
            }));
            menu->add(MainMenu::Action("Power Off", assets::icons_64::power_off, [](){
                // TODO power off
            }));
            c_.setMenu(menu);
            c_.setItem(0, Direction::Up);
        }

        ~HomeMenu() {

        }

    protected:
        void update() override {
            c_.processEvents();
            g_.update();

            if (btnPressed(Btn::B)) {
                btnClear(Btn::B);
                exit();
            };
        }

    private:

        ui::CarouselMenu c_;

    }; // rckid::HomeMenu

} // namespace rckid