#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/graphics/bitmap.h>
#include <rckid/graphics/png.h>
#include <rckid/assets/fonts/Iosevka16.h>
#include <rckid/assets/images.h>

using namespace rckid;

class DisplayPNGApp : public App<RenderableBitmap<ColorRGB>> {
public:
    DisplayPNGApp(Coord w, Coord h): App{RenderableBitmap<ColorRGB>{w, h}} {
        g_.loadImage(PNG::fromBuffer(assets::logo256));
    }

    static void run() { DisplayPNGApp t{320, 240}; t.loop(); }

protected:

    void draw() override {
        /*
        switch (++i_ % 4) {
            case 0:
                g_.fill(ColorRGB{255, 0, 0});
                break;
            case 1:
                g_.fill(ColorRGB{0, 255, 0});
                break;
            case 2:
                g_.fill(ColorRGB{0, 0, 255});
                break;
            case 3:
                g_.fill(ColorRGB{0, 0, 0});
                break;
        }
        g_.text(10,10, Font::fromROM<assets::Iosevka16>(), Color::RGB(0xff, 0xff, 0xff)) << "Hello world!";
        */
        App::draw();
    }

    uint32_t i_ = 0;
}; 

int main() {
    initialize();
    while (true) {
        DisplayPNGApp::run();
    }
}