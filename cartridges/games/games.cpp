#include "rckid/rckid.h"
#include "rckid/apps/games/SlidingPuzzle.h"
#include "rckid/apps/games/Tetris.h"

using namespace rckid;

int main() {
    initialize();
    SlidingPuzzle game;
    game.run();
}
