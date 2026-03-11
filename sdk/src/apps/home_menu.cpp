#include <rckid/apps/home_menu.h>

#include <assets/icons_64.h>

namespace rckid {

    void HomeMenu::onLoopStart() {
        ui::App<ui::MenuItem::ActionEvent>::onLoopStart();
        ASSERT(parent() != nullptr);
        ui::with(carousel_)
            << ui::ResetMenu([app = parent(), this] () { 
                auto menu = app->homeMenu();
                (*menu)
                    << ui::MenuItem("Brightness", assets::icons_64::brightness, [this]() {
                        showProgressBar(0, 15, display::brightness() >> 4, [](int32_t value) {
                                display::setBrightness(value * 16 + value); 
                        });
                    }).withPayload(ExecuteInMenu)
                    << ui::MenuItem("Volume", assets::icons_64::high_volume, [this]() {
                        showProgressBar(0, 15, audio::volume(), [](int32_t value) { 
                            audio::setVolume(value); 
                        });
                    }).withPayload(ExecuteInMenu)
                    << ui::MenuItem("Power Off", assets::icons_64::power_off, []() {
                        //rckid::device::powerOff();
                    });
                if (app->parent() != nullptr)
                    (*menu)
                        << ui::MenuItem("Exit", assets::icons_64::logout, [app]() {
                            app->exit();
                        });
                return menu;
            });
    }

}