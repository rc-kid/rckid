#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/apps/MusicPlayer.h>
#include <rckid/apps/Friends.h>
#include <rckid/apps/Messages.h>
#include <gbcemu/gbcemu.h>
#include <rckid/apps/FMRadio.h>
#include <rckid/ui/system_menu.h>


#include <rckid/apps/Drawing.h>

using namespace rckid;

ui::ActionMenu * gamesGenerator() {
    auto result = ui::gamesMenuGenerator();
    // append available gbcemu ROMs
    gbcemu::GBCEmu::appendGamesFrom("/games", result);
    return result;
}

ui::ActionMenu * mainMenu() {
    std::unordered_set<String> blacklist{ui::getBlacklistedApps()};
    auto result = new ui::ActionMenu{};
    result->add(ui::ActionMenu::Generator("Games", assets::icons_64::game_controller, gamesGenerator));
    ui::addAppToMenu(result, "Music", assets::icons_64::music, App::run<MusicPlayer>, blacklist);
    ui::addAppToMenu(result, "Radio", assets::icons_64::radio_cassette, App::run<FMRadio>, blacklist);
    ui::addAppToMenu(result, "Friends", assets::icons_64::birthday_cake, App::run<Friends>, blacklist);
    ui::addAppToMenu(result, "Messages", assets::icons_64::chat, App::run<Messages>, blacklist);
    // TODO should drawing move to its own submenu with other image/asset stuff? 
    ui::addAppToMenu(result, "Drawing", assets::icons_64::paint_palette, App::run<Drawing>, blacklist);
    result->add(ui::ActionMenu::Generator("Audio", assets::icons_64::music_2, ui::audioMenuGenerator));
    result->add(ui::ActionMenu::Generator("Utilities", assets::icons_64::configuration, ui::utilsMenuGenerator));
    result->add(ui::ActionMenu::Generator("Settings", assets::icons_64::settings, ui::settingsMenuGenerator));
    if (debugMode())
        result->add(ui::ActionMenu::Generator("Devel", assets::icons_64::ladybug, ui::develMenuGenerator));
    return result;
}

int main() {
    //cpu::overclock(250000000);
    initialize();
    ui::runSystemMenu(mainMenu);
}

