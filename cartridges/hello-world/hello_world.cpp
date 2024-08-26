#include <rckid/rckid.h>

#include <games/SlidingPuzzle.h>
#include <games/Pong.h>

int main() {
    rckid::initialize();
    LOG("Initialized, running the app!");
    auto game = rckid::Pong::create();
    game->run();
    while (true) {
        rckid::tick();
    };
}