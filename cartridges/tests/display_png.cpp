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
        decTime_ = MEASURE_TIME(
            g_.loadImage(PNG::fromBuffer(assets::logo256));
        );
        LOG(LL_INFO, "Decoding time: " << decTime_);
        g_.text(10,10, Font::fromROM<assets::Iosevka16>(), Color::RGB(0xff, 0xff, 0xff)) << "Decoding time: " << decTime_;
    }

    static void run() { DisplayPNGApp t{320, 240}; t.loop(); }

protected:

    void draw() override {
        App::draw();
    }

    uint32_t decTime_;
}; 

int main() {
    initialize();
    while (true) {
        DisplayPNGApp::run();
    }
}