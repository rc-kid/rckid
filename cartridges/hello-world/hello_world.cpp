


#include <rckid/rckid.h>

#include <rckid/graphics/tile.h>

#include <games/Pong.h>
#include <games/Tetris.h>
#include <games/SlidingPuzzle.h>
#include <games/GalaxyInvaders.h>
#include <games/Sokoban.h>
#include <test/RawAudioTest.h>
#include <test/ToneAudioTest.h>
#include <utils/StatusDisplay.h>
#include <utils/StatusDisplay.h>
#include <utils/Clock.h>
#include <utils/Stopwatch.h>
#include <utils/SetTime.h>
#include <utils/SettingsGauge.h>
#include <rckid/apps/DataSync.h>

#include <benchmarks/ToneGenerator.h>
#include <benchmarks/MP3Decoder.h>

#include <rckid/ui/menu.h>
#include <rckid/ui/menu_app.h>


//#include <rckid/audio/opus.h>
#include <rckid/apps/AudioPlayer.h>
//#include <rckid/graphics/tile_engine.h>

#include <rckid/ui/ui.h>
#include <rckid/ui/text_input.h>

#include <rckid/comms/uart_transceiver.h>
//#include <rckid/comms/connection.h>

#include <rckid/assets/icons64.h>


#include <rckid/filesystem.h>

using namespace rckid;

Menu * menuGames() {
    return new Menu{
        MenuApp::Item("Pong", assets::icons64::ping_pong, Pong::run),
        MenuApp::Item("Tetris", assets::icons64::tetris, Tetris::run),
        MenuApp::Item("Sliding Puzzle", assets::icons64::mosaic, SlidingPuzzle::run),
        MenuApp::Item("Invaders", assets::icons64::space, GalaxyInvaders::run),
        MenuApp::Item("Sokoban", assets::icons64::wooden_box, Sokoban::run),
    };
}

Menu * menuUtils() {
    Menu * m = new Menu{
        MenuApp::Item("Stopwatch", assets::icons64::clock, Stopwatch::run),
        MenuApp::Item("Clock", assets::icons64::alarm_clock, Clock::run),
    };
    if (debugMode()) {
        m->add(MenuApp::Item("Data Sync", assets::icons64::pen_drive, DataSync::run));
        m->add(MenuApp::Item("Status", assets::icons64::info, StatusDisplay::run));
        //MenuApp::Item("Raw Audio", assets::icons64::music, RawAudioTest::run),
        //MenuApp::Item("Tone Audio", assets::icons64::music, ToneAudioTest::run),
    }
    return m;
}

Menu * menuSettings() {
    return new Menu{
        MenuApp::Item("Date & Time", assets::icons64::clock, SetTime::run),
        MenuApp::Item("Brightness", assets::icons64::brightness, setBrightness),
        MenuApp::Item("Volume", assets::icons64::high_volume, setVolume),
    };
}

Menu * menuBenchmarks() {
    return new Menu{
        MenuApp::Item("Tone Generator", assets::icons64::music, Benchmark<ToneGeneratorBenchmark>::run),
        MenuApp::Item("MP3 Decoder", assets::icons64::music, Benchmark<MP3DecoderBenchmark>::run),
    };
}

Menu * mainMenu() {
    Menu * m = new Menu{
        MenuApp::Submenu("Games", assets::icons64::game_controller, menuGames),
        MenuApp::Item("Music", assets::icons64::music, AudioPlayer::run),
        MenuApp::Submenu("Utils", assets::icons64::applications, menuUtils),
        MenuApp::Submenu("Settings", assets::icons64::settings, menuSettings),
    };
    if (debugMode()) {
        m->add(MenuApp::Submenu("Benchmarks", assets::icons64::spider, menuBenchmarks));
    }
    return m;
}

int main() {
    namespace fs = rckid::filesystem;
    rckid::overclock();
    rckid::initialize();
    LOG("Initialized, running the app!");
    rckid::displaySetBrightness(128);

    //rckid::rumbleOk();
    while (true)
        MenuApp::run(mainMenu);
    while (true) {
        rckid::tick();
    };
}
