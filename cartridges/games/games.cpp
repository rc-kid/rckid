#include "rckid/rckid.h"
#include "rckid/apps/system/Carousel.h"
#include "rckid/apps/games/SlidingPuzzle.h"
#include "rckid/apps/games/Tetris.h"

#include "rckid/graphics/bitmap.h"
#include "rckid/audio.h"

using namespace rckid;

void rckid_main() {
    audio::setAudioEnabled(true);
    //start(Menu{});
    SlidingPuzzle{}.run();
    //start(Tetris{});
    //Menu menu;
    //menu.run();
    //SlidingPuzzle game;
    //game.run();
}
