#pragma once

#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/carousel.h"
#include "../ui/menu.h"

namespace rckid {

    /** Main menu application.
     
        The app is responsible for displaying the app launcher menu as well as header and basic user information.
     */
    class MainMenu : public ui::App {
    public:
        MainMenu() : ui::App{} {

        }

    private:
        ui::CarouselMenu * c_;


    }; // rckid::MainMenu


} // namespace rckid