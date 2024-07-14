
#include "rckid/rckid.h"
#include "rckid/audio.h"

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

#include <iostream>

#include "lib/gbc.h"

using namespace rckid;

class GBCEmu : public App<FrameBuffer<ColorRGB>> {
public:

    GBCEmu() {
        DEBUG("GBCEmu started");
    }

    void update() override {
        GBC gbc{};
        gbc.start();
    }

    void draw() override {
        driver_.fill(Rect::XYWH(40, 12, 240, 216));
    }

    void onFocus() override {
        App::onFocus();
        driver_.setBg(ColorRGB::Black());
        driver_.fill();
        driver_.setBg(ColorRGB::Blue());
    }
}; 


void rckid_main() {
    audio::setAudioEnabled(true);
    GBCEmu{}.run();
    //start(Menu{});
    //start(SlidingPuzzle{});
    //Menu menu;
    //menu.run();
    //SlidingPuzzle game;
    //game.run();
}
