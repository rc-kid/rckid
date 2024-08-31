#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/assets/icons.h>
#include <rckid/assets/fonts/Iosevka16.h>

using namespace rckid;

class TestAppRGB : public GraphicsApp<Bitmap<ColorRGB>> {
public:
    TestAppRGB(Coord w, Coord h): GraphicsApp{Bitmap<ColorRGB>{w, h}} {}

protected:

    void draw() override {
        switch (i_) {
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
        i_ = (i_ + 1) % 4;
        Font font(Font::fromROM<assets::font::Iosevka16>());
        g_.text(0,0,font, ColorRGB{255, 255, 255}) 
            << "Hello world!\n" 
            << "FPS: " << App::fps() << "\n"
            << "Draw: " << App::drawUs() << "\n"
            << "Render: " << App::renderUs() << "\n";
        MemoryReadStream str{rckid::assets::icons::bat};
        //g_.loadImage(PNG::fromStream(str));
    }
    uint32_t i_ = 0;
}; 

int main() {
    initialize();
    while (true) {
        TestAppRGB{200, 200}.run();
    }
}