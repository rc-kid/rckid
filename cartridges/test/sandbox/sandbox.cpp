#define HAHA_

#ifdef HAHA

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/font.h"
#include "assets/fonts/Iosevka_16.h"

#include "rckid/ui/menu_app.h"

#include "apps/games/Pong.h"
#include "apps/games/SlidingPuzzle.h"

#include "rckid/fs/USBMassStorage.h"
#include "rckid/system/HWTest.h"

#include "assets/all.h"

#include "rckid/audio/tone.h"
#include "rckid/audio/music.h"


#include "assets/fonts/OpenDyslexic_16.h"
#include "assets/fonts/Hasklug_16.h"
#include "assets/fonts/Hurmit_16.h"
#include "assets/fonts/VictorMono_16.h"
#include "assets/fonts/VictorMonoBold_16.h"
#include "assets/fonts/Lilly_24.h"
#include "assets/fonts/Inconsolata_24.h"
#include "assets/fonts/PixelFJVerdana_24.h"
#include "assets/fonts/DPComic_24.h"



#include "apps/debug/NRFSniffer.h"
#include "apps/debug/MP3Test.h"
#include "apps/debug/WAVTest.h"
#include "apps/debug/MicTest.h"
#include "apps/debug/melody.h"

using namespace rckid;

class FontSelector : public App<FrameBuffer<ColorRGB>> {
public:
    static FontSelector * create() { return new FontSelector{}; }

protected:

    void draw() override {
        driver_.fill();
        std::string text = "Q W E R T Y U I O P [ ]";
        driver_.text(0,0, Iosevka_16, Color::White()) << text;
        driver_.text(0,20, OpenDyslexic_16, Color::White()) << text;
        driver_.text(0,40, Hasklug_16, Color::White()) << text;
        driver_.text(0,60, Hurmit_16, Color::White()) << text;
        driver_.text(0,80, VictorMono_16, Color::White()) << text;
        driver_.text(0,100, VictorMonoBold_16, Color::White()) << text;
        driver_.text(0,120, Lilly_24, Color::White()) << text;
        driver_.text(0,150, Inconsolata_24, Color::White()) << text;
        driver_.text(0,180, PixelFJVerdana_24, Color::White()) << text;
        driver_.text(0,210, DPComic_24, Color::White()) << text;
    }
}; 


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

Menu * menuSettings() {
    return new Menu{{
        MenuItem::create("Volume", assets::icons::high_volume),
        MenuItem::create("Brightness", assets::icons::brightness),
    }};
}

Menu * menuDebug() {
    return new Menu{{
        MenuItem::create("HW Test", assets::icons::freesia, HWTest::create),
        MenuItem::create("Fonts", assets::icons::freesia, FontSelector::create),
        MenuItem::create("NRFSniffer", assets::icons::rabbit_1, NRFSniffer::create),
        MenuItem::create("MP3 Test", assets::icons::rabbit_1, MP3Test::create),
        MenuItem::create("Melody", assets::icons::music, Melody::create),
        MenuItem::create("WAV Test", assets::icons::rabbit_1, WAVTest::create),
        MenuItem::create("Mic Test", assets::icons::microphone, MicTest::create),
    }};
}


int main() {
    rckid::initialize();
    cpu::overclock();
    
    StaticMenuStack<> menu{
        new Menu{{
            MenuItem::createSubmenu("Games", assets::icons::game_controller, menuGames), 
            MenuItem::create("Music", assets::icons::music), 
            MenuItem::create("Walkie-Talkie", assets::icons::baby_monitor), 
            MenuItem::createSubmenu("Settings", assets::icons::settings, menuSettings),
            MenuItem::createSubmenu("Utils", assets::icons::applications, menuUtils),
            MenuItem::createSubmenu("Debug", assets::icons::lynx, menuDebug),
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
