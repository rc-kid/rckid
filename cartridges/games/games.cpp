#include "rckid/rckid.h"
#include "rckid/apps/system/Carousel.h"
#include "rckid/apps/games/SlidingPuzzle.h"
#include "rckid/apps/games/Tetris.h"

#include "rckid/graphics/bitmap.h"
#include "rckid/Audio.h"

using namespace rckid;

int main() {
    initialize();
    Audio::setAudioEnabled(true);
    //start(Menu{});
    start(SlidingPuzzle{});
    //Menu menu;
    //menu.run();
    //SlidingPuzzle game;
    //game.run();
}
