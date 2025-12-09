#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/apps/MusicPlayer.h>
#include <rckid/apps/Friends.h>
#include <rckid/apps/Messages.h>
#include <gbcemu/gbcemu.h>
#include <rckid/apps/FMRadio.h>
#include <rckid/ui/system_menu.h>

using namespace rckid;

ui::ActionMenu * gamesGenerator() {
    auto result = ui::gamesMenuGenerator();
    // append available gbcemu ROMs
    gbcemu::GBCEmu::appendGamesFrom("/games", result);
    return result;
}

ui::ActionMenu * mainMenu() {
    auto result = new ui::ActionMenu{
        ui::ActionMenu::Generator("Games", assets::icons_64::game_controller, gamesGenerator),
        ui::ActionMenu::Item("Music", assets::icons_64::music, App::run<MusicPlayer>),
        ui::ActionMenu::Item("Radio", assets::icons_64::radio_cassette, App::run<FMRadio>),
        ui::ActionMenu::Item("Friends", assets::icons_64::birthday_cake, App::run<Friends>),
        ui::ActionMenu::Item("Messages", assets::icons_64::chat, App::run<Messages>),
        ui::ActionMenu::Generator("Utilities", assets::icons_64::configuration, ui::utilsMenuGenerator),
        ui::ActionMenu::Generator("Settings", assets::icons_64::settings, ui::settingsMenuGenerator),
    };
    if (debugMode()) {
        result->add(ui::ActionMenu::Generator("Devel", assets::icons_64::ladybug, ui::develMenuGenerator));
    }
    return result;
}

int main() {
    //cpu::overclock(250000000);
    initialize();
    ui::runSystemMenu(mainMenu);
}

