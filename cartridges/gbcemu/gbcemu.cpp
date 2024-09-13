
#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/graphics/canvas.h>

#include "lib/gbc.h"

using namespace rckid;

class GBCEmu : public GraphicsApp<Canvas<ColorRGB>> {
public:

    GBCEmu():
        GraphicsApp{Canvas<ColorRGB>{160, 144}}
    {
        LOG("GBCEmu started");
    }

    void update() override {
        //GBC gbc{};
        //gbc.start();
    }

    void draw() override {
        g_.fill(Rect::XYWH(40, 12, 240, 216));
    }

    void onFocus() override {
        App::onFocus();
        g_.setBg(color::Black);
        g_.fill();
        g_.setBg(color::Blue);
    }
}; 


int main() {
    rckid::initialize();
    audioEnable();
    GBCEmu{}.run();
    //start(Menu{});
    //start(SlidingPuzzle{});
    //Menu menu;
    //menu.run();
    //SlidingPuzzle game;
    //game.run();
}
