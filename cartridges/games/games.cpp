#include "rckid/rckid.h"
#include "rckid/apps/system/Carousel.h"
#include "rckid/apps/games/SlidingPuzzle.h"
#include "rckid/apps/games/Tetris.h"

using namespace rckid;

int main() {
    initialize();
    Menu menu;
    menu.run();
    //SlidingPuzzle game;
    //game.run();
}
