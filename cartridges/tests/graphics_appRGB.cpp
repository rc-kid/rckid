#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/graphics/bitmap.h>
#include <rckid/assets/fonts/Iosevka16.h>

using namespace rckid;
class TestAppRGB : public BitmapApp<ColorRGB> {
public:
    TestAppRGB(Coord w, Coord h): BitmapApp<ColorRGB>{RenderableBitmap<ColorRGB>{w, h}} {}

protected:

    void draw() override {
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
        std::array<Bitmap<ColorRGB>::Pixel, 4> colors = {
            ColorRGB{0, 0, 0},
            ColorRGB{85, 85, 80},
            ColorRGB{170, 170, 170},
            ColorRGB{255, 255, 255}
        };
        g_.text(10,10, Font::fromROM<assets::Iosevka16>(), ColorRGB{255, 255, 255}) << "Hello world!";
        BitmapApp<ColorRGB>::draw();
    }

    uint32_t i_ = 0;
}; 

int main() {
    initialize();
    while (true) {
        auto t = TestAppRGB{200, 200};
        t.run();
    }
}

