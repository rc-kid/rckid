#pragma once

#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/carousel.h"
#include "../ui/menu.h"

namespace rckid {

    /** Main menu application. 
     */
    class MainMenu : public ui::App {
    public:
        MainMenu() : ui::App{} {

        }

    private:
        ui::CarouselMenu * c_;


    }; // rckid::MainMenu


} // namespace rckid