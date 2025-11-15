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
#include <rckid/apps/dialogs/PinDialog.h>
#include <rckid/apps/dialogs/FileDialog.h>
#include <rckid/apps/dialogs/InfoDialog.h>
#include <rckid/apps/dialogs/PopupMenu.h>
#include <rckid/apps/dialogs/ColorPicker.h>
#include <rckid/apps/MusicPlayer.h>
#include <rckid/apps/Friends.h>
#include <rckid/apps/Messages.h>
#include <rckid/apps/DataSync.h>
#include <rckid/apps/Recorder.h>
#include <rckid/apps/Drawing.h>
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
        //ui::ActionMenu::Item("Files", assets::icons_64::folder, App::run<FileDialog>),
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
        ui::ActionMenu::Item("Drawing", assets::icons_64::paint_palette, App::run<Drawing>),
        // TODO browser for images alone
    };
}

ui::ActionMenu * styleMenuGenerator() {
    return new ui::ActionMenu{
        ui::ActionMenu::Item("Text", assets::icons_64::color_picker, []() {
            auto c = App::run<ColorPicker>(ui::Style::fg());
            if (c.has_value()) {
                ui::Style::setFg(c.value());
                ui::Style::refreshAndSave();
            }
        }),
        ui::ActionMenu::Item("Bg", assets::icons_64::color_picker, [](){
            auto c = App::run<ColorPicker>(ui::Style::bg());
            if (c.has_value()) {
                ui::Style::setBg(c.value());
                ui::Style::refreshAndSave();
            }
        }),
        ui::ActionMenu::Item("Accent Text", assets::icons_64::color_picker, []() {
            auto c = App::run<ColorPicker>(ui::Style::accentFg());
            if (c.has_value()) {
                ui::Style::setAccentFg(c.value());
                ui::Style::refreshAndSave();
            }
        }),
        ui::ActionMenu::Item("Accent Bg", assets::icons_64::color_picker, [](){
            auto c = App::run<ColorPicker>(ui::Style::accentBg());
            if (c.has_value()) {
                ui::Style::setAccentBg(c.value());
                ui::Style::refreshAndSave();
            }
        }),
        ui::ActionMenu::Item("Background", assets::icons_64::picture, [](){
            auto icon = App::run<FileDialog>("/files/images");
            if (icon.has_value()) {
                ui::Style::setBackground(Icon{icon.value().c_str()});
                ui::Style::refreshAndSave();
            }
        }),
    };
}

ui::ActionMenu * settingsMenuGenerator() {
    return new ui::ActionMenu{
        ui::ActionMenu::Generator("Style", assets::icons_64::paint_palette, styleMenuGenerator),
        ui::ActionMenu::Item("Pin", assets::icons_64::lock, [](){
            if (pinCurrent() != 0xffff) {
                auto x = App::run<PinDialog>("Current pin");
                if (! x.has_value())
                    return;
                if (x.value() != pinCurrent()) {
                    InfoDialog::error("Incorrect PIN", "The PIN you entered is incorrect.");
                    return;
                }
            }
            auto x = App::run<PinDialog>("New pin");
            if (! x.has_value())
                return;
            uint16_t newPin = x.value();
            x = App::run<PinDialog>("New pin again");
            if (! x.has_value())
                return;
            if (x.value() != newPin) {
                InfoDialog::error("PIN mismatch", "The new PIN entries do not match.");
                return;
            }
            pinSet(newPin);
            InfoDialog::success("Done", "PIN changed successfully");
        }),
        ui::ActionMenu::Item("RGB Off", assets::icons_64::turn_off, [](){
            rckid::rgbOff();
        }),
        ui::ActionMenu::Item("RGB Rainbow", assets::icons_64::rainbow, [](){
            rckid::rgbEffects(
                RGBEffect::Rainbow(0, 1, 4, 32), 
                RGBEffect::Rainbow(51, 1, 4, 32), 
                RGBEffect::Rainbow(102, 1, 4, 32), 
                RGBEffect::Rainbow(153, 1, 4, 32), 
                RGBEffect::Rainbow(204, 1, 4, 32)
            );
        }),
    };
}

ui::ActionMenu * debugMenuGenerator() {
    return new ui::ActionMenu{
        ui::ActionMenu::Item("Debug Off", assets::icons_64::ladybug, [](){
            rckid::setDebugMode(false);
        }),
        ui::ActionMenu::Item("Debug On", assets::icons_64::ladybug, [](){
            rckid::setDebugMode(true);
        }),
    };
}


ui::ActionMenu * mainMenuGenerator() {
    return new ui::ActionMenu{
        //ui::ActionMenu::Item("Test", assets::icons_64::heart, App::run<Messages::ConversationView>),
        ui::ActionMenu::Generator("Games", assets::icons_64::game_controller, gamesGenerator),
        ui::ActionMenu::Item("Music", assets::icons_64::music, App::run<MusicPlayer>),
        ui::ActionMenu::Item("Radio", assets::icons_64::radio_cassette, App::run<FMRadio>),
        ui::ActionMenu::Generator("Comms", assets::icons_64::chat, commsMenuGenerator),
        ui::ActionMenu::Generator("Audio", assets::icons_64::music_wave, audioMenuGenerator),
        ui::ActionMenu::Generator("Images", assets::icons_64::picture, imagesMenuGenerator),
        ui::ActionMenu::Item("Remote", assets::icons_64::rc_car, nullptr),
        ui::ActionMenu::Generator("Utilities", assets::icons_64::configuration, utilsMenuGenerator),
        ui::ActionMenu::Generator("Settings", assets::icons_64::settings, settingsMenuGenerator),
        ui::ActionMenu::Generator("Debug", assets::icons_64::ladybug, debugMenuGenerator),
    };
}


int main() {
    //cpu::overclock(250000000);
    initialize();
    LOG(LL_INFO, "Init done, rgb effects");
    rgbEffects(
        RGBEffect::Rainbow(0, 1, 4, 32), 
        RGBEffect::Rainbow(51, 1, 4, 32), 
        RGBEffect::Rainbow(102, 1, 4, 32), 
        RGBEffect::Rainbow(153, 1, 4, 32), 
        RGBEffect::Rainbow(204, 1, 4, 32)
    );
    LOG(LL_INFO, "RGB effects sent");
    //App::run<TextDialog>("Input text");
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

