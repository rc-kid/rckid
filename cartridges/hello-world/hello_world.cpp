#include <games/Tetris.h>



#include <rckid/rckid.h>

#include <rckid/graphics/tile.h>

#include <games/SlidingPuzzle.h>
#include <games/Pong.h>
#include <test/RawAudioTest.h>

#include <rckid/ui/menu.h>



//#include <rckid/graphics/tile_engine.h>

#include <rckid/ui/ui.h>
#include <rckid/ui/text_input.h>

#include <rckid/comms/uart_transceiver.h>
//#include <rckid/comms/connection.h>


using namespace rckid;

int main() {
    rckid::initialize();
    LOG("Initialized, running the app!");
    //auto game = rckid::SlidingPuzzle::create();
    //auto game = new rckid::RawAudioTest{};
    //auto game = new TextInput{};
    auto game = Tetris::create();
    game->run();
    while (true) {
        rckid::tick();
    };
}



/*

- have playfield
- randomize, then spawn
- spawn tetromino
- rotations == create a cpy, rotate, if validated, swap







*/