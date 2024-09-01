#include <rckid/rckid.h>

#include <games/SlidingPuzzle.h>
#include <games/Pong.h>
#include <test/RawAudioTest.h>

int main() {
    rckid::initialize();
    LOG("Initialized, running the app!");
    //auto game = rckid::SlidingPuzzle::create();
    auto game = new rckid::RawAudioTest{};
    game->run();
    while (true) {
        rckid::tick();
    };
}