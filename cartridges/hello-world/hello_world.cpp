#include <rckid/rckid.h>

#include <rckid/graphics/tile.h>

#include <games/SlidingPuzzle.h>
#include <games/Pong.h>
#include <test/RawAudioTest.h>

using namespace rckid;

int main() {
//    Tile<1,8,Color16> t;
    rckid::initialize();
    LOG("Initialized, running the app!");
    //auto game = rckid::SlidingPuzzle::create();
    auto game = new rckid::RawAudioTest{};
    game->run();
    while (true) {
        rckid::tick();
    };
}