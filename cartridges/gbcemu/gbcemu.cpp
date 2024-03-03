
#include "rckid/rckid.h"
#include "rckid/audio.h"

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

#include <iostream>

#include "lib/gbc.h"

using namespace rckid;

class GBCEmu : public FrameBufferApp<ColorRGB> {
public:

    GBCEmu() {
        DEBUG("GBCEmu started");
    }

    void update() override {
        GBC gbc{};
        gbc.start();
    }

    void draw() override {
        fb_.fill(Rect::XYWH(40, 12, 240, 216));
    }

    void onFocus() override {
        FrameBufferApp::onFocus();
        fb_.setBg(ColorRGB::Black());
        fb_.fill();
        fb_.setBg(ColorRGB::Blue());
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
