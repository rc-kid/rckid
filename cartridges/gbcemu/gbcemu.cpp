
#include "rckid/rckid.h"
#include "rckid/Audio.h"

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"


#include "gbc.h"

using namespace rckid;

class GBCEmu : public App<FrameBuffer<ColorRGB>> {
public:

    void update() override {

    }

    void draw() override {
        Renderer & r = renderer();
        r.fill(Rect::XYWH(40, 12, 240, 216));
    }

    void onFocus(BaseApp * previous) override {
        App::onFocus(previous);
        Renderer & r = renderer();
        r.setBg(ColorRGB::Black());
        r.fill();
        r.setBg(ColorRGB::Blue());
    }
}; 


int main() {
    initialize();
    Audio::setAudioEnabled(true);
    start(GBCEmu{});
    //start(Menu{});
    //start(SlidingPuzzle{});
    //Menu menu;
    //menu.run();
    //SlidingPuzzle game;
    //game.run();
}
