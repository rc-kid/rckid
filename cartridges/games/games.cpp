#include "rckid/rckid.h"
#include "apps/games/SlidingPuzzle.h"

using namespace rckid;

int main() {
    rckid::initialize();
    //start(Menu{});
    SlidingPuzzle{}.run();
    //start(Tetris{});
    //Menu menu;
    //menu.run();
    //SlidingPuzzle game;
    //game.run();
}
