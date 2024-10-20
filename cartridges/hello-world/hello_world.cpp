


#include <rckid/rckid.h>

#include <rckid/graphics/tile.h>

#include <games/Pong.h>
#include <games/Tetris.h>
#include <games/SlidingPuzzle.h>
#include <test/RawAudioTest.h>
#include <test/ToneAudioTest.h>
#include <utils/StatusDisplay.h>

#include <benchmarks/ToneGenerator.h>

#include <rckid/ui/menu.h>
#include <rckid/ui/menu_app.h>



//#include <rckid/graphics/tile_engine.h>

#include <rckid/ui/ui.h>
#include <rckid/ui/text_input.h>

#include <rckid/comms/uart_transceiver.h>
//#include <rckid/comms/connection.h>

#include <rckid/assets/icons.h>


using namespace rckid;


Menu * menuGames() {
    return new Menu{
        MenuApp::Item("Pong", assets::icons::bat, Pong::run),
        MenuApp::Item("Tetris", assets::icons::beaver, Tetris::run),
        MenuApp::Item("Sliding Puzzle", assets::icons::bee, SlidingPuzzle::run),
        //MenuApp::Item("Space Invaders", assets::icons::bird_of_paradise, nullptr),
    };
}

Menu * menuUtils() {
    return new Menu{
        MenuApp::Item("Status", assets::icons::info, StatusDisplay::run),
        //MenuApp::Item("Raw Audio", assets::icons::music, RawAudioTest::run),
        MenuApp::Item("Tone Audio", assets::icons::music, ToneAudioTest::run)
    };
}

Menu * menuBenchmarks() {
    return new Menu{
        MenuApp::Item("Tone Generator", assets::icons::music, Benchmark<ToneGenerator>::run),
    };
}


Menu * mainMenu() {
    return new Menu{
        MenuApp::Submenu("Games", assets::icons::game_controller, menuGames),
        MenuApp::Submenu("Utils", assets::icons::applications, menuUtils),
        MenuApp::Submenu("Settings", assets::icons::settings, menuGames),
        MenuApp::Submenu("Benchmarks", assets::icons::spider, menuBenchmarks),
    };
}

int main() {
    //cpu::overclock();
    rckid::initialize();
    LOG("Initialized, running the app!");
    rckid::displaySetBrightness(128);
    rckid::rumbleOk();
    //auto game = rckid::SlidingPuzzle::create();
    //auto game = new rckid::RawAudioTest{};
    //auto game = new TextInput{};
    //auto game = Tetris::create();
    //game->run();
    while (true)
        MenuApp::run(mainMenu);
    while (true) {
        rckid::tick();
    };
}
