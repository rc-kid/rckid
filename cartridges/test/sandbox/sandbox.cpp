#define HAHA_

#ifdef HAHA

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/font.h"
#include "assets/fonts/Iosevka_16.h"

//#include "apps/system/MainMenu.h"
#include "rckid/ui/menu_app.h"

#include "apps/games/Pong.h"
#include "apps/games/SlidingPuzzle.h"

#include "rckid/system/USBMassStorage.h"

#include "assets/all.h"

#include "rckid/audio/tone.h"
#include "rckid/audio/music.h"

using namespace rckid;

/** The app games menu
 */
Menu * menuGames() {
    return new Menu{{
        MenuItem::create("Pong", assets::icons::fruits, Pong::create),
        MenuItem::create("SlidingPuzzle", assets::icons::lynx, SlidingPuzzle::create),
    }};
}

Menu * menuUtils() {
    return new Menu{{
        MenuItem::create("Data Sync", assets::icons::freesia, USBMassStorage::create),
    }};

}


int main() {
    rckid::initialize();
    StaticMenuStack<> menu{
        new Menu{{
            MenuItem::createSubmenu("Games", assets::icons::game_controller, menuGames), 
            MenuItem::create("Music", assets::icons::music), 
            MenuItem::create("Walkie-Talkie", assets::icons::baby_monitor), 
            MenuItem::create("Settings", assets::icons::settings),
            MenuItem::createSubmenu("Utils", assets::icons::applications, menuUtils),
            MenuItem::create("Development", assets::icons::lynx),
        }}
    };

    while (true) {
        enterHeapArena();
        std::optional<void*> launcher = MenuApp<ColorRGB>(menu).run();
        leaveHeapArena();
        ASSERT(launcher.has_value());

        enterHeapArena();
        BaseApp * app = reinterpret_cast<AppLauncher>(launcher.value())();
        app->run();
        delete app;
        leaveHeapArena();
    }

}

#endif
