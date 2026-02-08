#pragma once

#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>

namespace rckid {

    /** Simple music player. 
     */
    class Music : public ui::App<void> {
    public:
        Music():
            carousel_{Launcher::borrowCarousel()}
        {
            using namespace ui;
        }

    private:
        ui::CarouselMenu * carousel_;


    }; // rckid::Music

} // namespace rckid