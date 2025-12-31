#include "rckid-default.h"


#include <rckid/apps/devel/MicTest.h>
#include <rckid/apps/utils/Steps.h>


using namespace rckid;


ui::ActionMenu * alphaMenu() {
    auto result = mainMenu();
    // insert mic test at the start
    result->add(ui::ActionMenu::Item("Mic test", assets::icons_64::microphone, App::run<MicTest>));
    result->add(ui::ActionMenu::Item("Steps", assets::icons_64::footprint, App::run<Steps>));
    return result;
}


int main() {
    initialize();
    Alarm::checkEvent();
    ui::runSystemMenu(alphaMenu);
}

