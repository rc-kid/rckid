#pragma once

#include "menu.h"

namespace rckid::ui {

    void runSystemMenu(ActionMenu::MenuGenerator menuGenerator);

    ActionMenu * mainMenuGenerator();

    ActionMenu * gamesMenuGenerator();
    ActionMenu * utilsMenuGenerator();
    ActionMenu * commsMenuGenerator();
    ActionMenu * audioMenuGenerator();
    ActionMenu * imagesMenuGenerator();
    ActionMenu * develMenuGenerator();

    ActionMenu * settingsMenuGenerator();


} // namespace rckid::ui