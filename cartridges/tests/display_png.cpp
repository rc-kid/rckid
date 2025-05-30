#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/graphics/bitmap.h>
#include <rckid/graphics/png.h>
#include <rckid/assets/fonts/Iosevka16.h>
#include <rckid/assets/images.h>


using namespace rckid;

class DisplayPNGApp : public BitmapApp<ColorRGB> {
public:
    DisplayPNGApp(Coord w, Coord h): BitmapApp<ColorRGB>{RenderableBitmap<ColorRGB>{w, h}} {
        decTime_ = MEASURE_TIME(
            g_.loadImage(PNG::fromBuffer(assets::logo256));
        );
        LOG(LL_INFO, "Decoding time: " << decTime_);
        g_.text(10,10, Font::fromROM<assets::Iosevka16>(), ColorRGB{255, 255, 255}) << "Decoding time: " << decTime_;
    }

protected:

    void draw() override {
        BitmapApp<ColorRGB>::draw();
    }

    uint32_t decTime_;
}; 

int main() {
    initialize();
    while (true) {
        auto app = DisplayPNGApp{320, 240};
        app.run();
    }
}
