#pragma once

#include <unordered_set>

#include "menu.h"

namespace rckid::ui {

    void runSystemMenu(ActionMenu::MenuGenerator menuGenerator);

    std::unordered_set<String> getBlacklistedApps();
    void addAppToMenu(ui::ActionMenu * menu, String const & appName, Icon const & icon, std::function<void()> launchFunc, std::unordered_set<String> const & blacklist);


    ActionMenu * mainMenuGenerator();

    ActionMenu * gamesMenuGenerator();
    ActionMenu * utilsMenuGenerator();
    ActionMenu * commsMenuGenerator();
    ActionMenu * audioMenuGenerator();
    ActionMenu * imagesMenuGenerator();
    ActionMenu * develMenuGenerator();

    ActionMenu * settingsMenuGenerator();


} // namespace rckid::ui