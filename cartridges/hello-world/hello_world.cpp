#include <rckid/rckid.h>

#include <rckid/graphics/tile.h>

#include <games/SlidingPuzzle.h>
#include <games/Pong.h>
#include <test/RawAudioTest.h>

#include <rckid/ui/menu.h>



//#include <rckid/graphics/tile_engine.h>

#include <rckid/ui/ui.h>
#include <rckid/ui/text_input.h>

using namespace rckid;

int main() {
    rckid::initialize();
    LOG("Initialized, running the app!");
    LOG(sizeof(Bitmap<Color16>));
    LOG(sizeof(Bitmap<Color256>));
    LOG(sizeof(Bitmap<ColorRGB>));
    //auto game = rckid::SlidingPuzzle::create();
    //auto game = new rckid::RawAudioTest{};
    auto game = new TextInput{};
    game->run();
    while (true) {
        rckid::tick();
    };
}