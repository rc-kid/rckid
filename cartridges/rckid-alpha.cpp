#include "rckid-default.h"

#include <rckid/heartbeat.h>


#include <rckid/apps/devel/MicTest.h>
#include <rckid/apps/utils/Steps.h>
#include <rckid/apps/utils/Level.h>
#include <rckid/apps/utils/IRRemote.h>

#include <rckid/apps/devel/RPBootloader.h>


using namespace rckid;


ui::ActionMenu * alphaMenu() {
    auto result = mainMenu();
    // insert mic test at the start
    result->add(ui::ActionMenu::Item("Mic test", assets::icons_64::microphone, App::run<MicTest>));
    result->add(ui::ActionMenu::Item("Level", assets::icons_64::level, App::run<Level>));
    result->add(ui::ActionMenu::Item("IR Remote", assets::icons_64::controller, App::run<IRRemote>));
    result->add(ui::ActionMenu::Item("RP Bootloader", assets::icons_64::microchip, App::run<RPBootloader>));
    return result;
}


int main() {
    Task::registerHeartbeatTask([]() -> Task* {
        return new rckid::HeartbeatTask{};
    });
    initialize();
    ASSERT(false);
    Alarm::checkEvent();
    ui::runSystemMenu(alphaMenu);
}

