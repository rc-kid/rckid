#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/ui/form.h>
#include <rckid/ui/panel.h>
#include <rckid/ui/image.h>
#include <rckid/assets/icons_64.h>
#include <rckid/assets/fonts/OpenDyslexic64.h>
#include <rckid/ui/label.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/menu.h>
#include <rckid/audio/tone.h>
#include <rckid/apps/MainMenu.h>

#include <rckid/ui/header.h>

#include <rckid/apps/dialogs/TextDialog.h>
#include <rckid/apps/dialogs/FileDialog.h>
#include <rckid/apps/dialogs/InfoDialog.h>
#include <rckid/apps/dialogs/PopupMenu.h>
#include <rckid/apps/MusicPlayer.h>
#include <rckid/apps/Friends.h>
#include <rckid/apps/Messages.h>
#include <rckid/apps/DataSync.h>
#include <rckid/apps/Recorder.h>
#include <rckid/apps/utils/Clock.h>
#include <rckid/apps/utils/Timer.h>
#include <rckid/apps/utils/Stopwatch.h>
#include <rckid/apps/utils/Flashlight.h>



#include <rckid/apps/devel/HardwareStatus.h>
#include <rckid/apps/devel/SDTest.h>
#include <rckid/apps/devel/WiFiScan.h>

#include <gbcemu/gbcemu.h>
#include <gbcemu/gamepak.h>

#include <rckid/apps/FMRadio.h>

#include <rckid/apps/games/Tetris.h>
#include <rckid/apps/games/Checkers.h>
//#include <rckid/apps/games/SlidingPuzzle.h>


using namespace rckid;

/** Game menu generator. 
 
    Returns the games available in the system. The games are either statically known, such as built-in games, or cartridge stored ROMs, or can be dynamic, by looking at roms in the games folder.
 */
ui::ActionMenu * gamesGenerator() {
    
    ui::ActionMenu * result = new ui::ActionMenu{
        //MainMenu::Action("Tetris", assets::icons_64::tetris, App::run<TextDialog>),
        ui::ActionMenu::Item("Tetris", assets::icons_64::tetris, App::run<Tetris>),
        ui::ActionMenu::Item("Checkers", assets::icons_64::poo, App::run<Checkers>),
    };
    // append available gbcemu ROMs
    gbcemu::GBCEmu::appendGamesFrom("/games", result);

    // and return the menu
    return result;
}

ui::ActionMenu * utilsMenuGenerator() {
    return new ui::ActionMenu{
        ui::ActionMenu::Item("Flashlight", assets::icons_64::flashlight, App::run<Flashlight>),
        ui::ActionMenu::Item("Clock", assets::icons_64::alarm_clock, App::run<Clock>),
        ui::ActionMenu::Item("Stopwatch", assets::icons_64::chronometer, App::run<Stopwatch>),
        ui::ActionMenu::Item("Timer", assets::icons_64::hourglass, nullptr),
        ui::ActionMenu::Item("Files", assets::icons_64::folder, App::run<FileDialog>),
        ui::ActionMenu::Item("Data Sync", assets::icons_64::pen_drive, App::run<DataSync>),
        ui::ActionMenu::Item("HW Status", assets::icons_64::pen_drive, App::run<HardwareStatus>),
        ui::ActionMenu::Item("SD Test", assets::icons_64::pen_drive, App::run<SDTest>),
        ui::ActionMenu::Item("WiFi Scan", assets::icons_64::pen_drive, App::run<WiFiScan>),
    };
}

ui::ActionMenu * commsMenuGenerator() {
    return new ui::ActionMenu{
        ui::ActionMenu::Item("Messages", assets::icons_64::chat, nullptr),
        ui::ActionMenu::Item("WalkieTalkie", assets::icons_64::baby_monitor, nullptr),
        ui::ActionMenu::Item("Friends", assets::icons_64::birthday_cake, App::run<Friends>),
    };
}

ui::ActionMenu * audioMenuGenerator() {
    return new ui::ActionMenu{
        ui::ActionMenu::Item("Recorder", assets::icons_64::microphone, App::run<Recorder>),
        ui::ActionMenu::Item("Composer", assets::icons_64::music_2, nullptr),
        // TODO browser for audio files alone
    };
}

ui::ActionMenu * imagesMenuGenerator() {
    return new ui::ActionMenu{
        ui::ActionMenu::Item("Drawing", assets::icons_64::paint_palette, nullptr),
        // TODO browser for images alone
    };
}

ui::ActionMenu * mainMenuGenerator() {
    return new ui::ActionMenu{
        ui::ActionMenu::Item("Test", assets::icons_64::heart, App::run<Messages::ConversationView>),
        ui::ActionMenu::Generator("Games", assets::icons_64::game_controller, gamesGenerator),
        ui::ActionMenu::Item("Music", assets::icons_64::music, App::run<MusicPlayer>),
        ui::ActionMenu::Item("Radio", assets::icons_64::radio_cassette, App::run<FMRadio>),
        ui::ActionMenu::Generator("Comms", assets::icons_64::chat, commsMenuGenerator),
        ui::ActionMenu::Generator("Audio", assets::icons_64::music_wave, audioMenuGenerator),
        ui::ActionMenu::Generator("Images", assets::icons_64::picture, imagesMenuGenerator),
        ui::ActionMenu::Item("Remote", assets::icons_64::rc_car, nullptr),
        ui::ActionMenu::Generator("Utilities", assets::icons_64::configuration, utilsMenuGenerator),
    };
}

int main() {
    //cpu::overclock(250000000);
    initialize();
    //PNG png{PNG::fromStream(fs::fileRead(STR("files/images/backgrounds/wish16.png")))};
    //LOG(LL_INFO, "PNG loaded: " << png.width() << "x" << png.height() << ", bpp: " << png.bpp());
    while (true) {
        LOG(LL_INFO, "Free memory: " << memoryFree() / 1024);
        auto app = App::run<MainMenu>(mainMenuGenerator);
        LOG(LL_INFO, "MainMenu done");
        yield();
        LOG(LL_INFO, "Free memory: " << memoryFree() / 1024);
        yield();
        ASSERT((app.has_value()));
        {
            RAMHeap::LeakGuard g_;
            app.value()();
        }
    }
}

