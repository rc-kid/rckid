#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/graphics/bitmap.h>
#include <rckid/assets/icons.h>
#include <rckid/assets/fonts/Iosevka16.h>

using namespace rckid;

class TestApp16 : public GraphicsApp<Bitmap<Color16>> {
public:
    TestApp16(Coord w, Coord h): GraphicsApp{Bitmap<Color16>{w, h}} {
        g_.setPalette(palette::generatePalette16());
    }

    ~TestApp16() noexcept override {
        delete [] g_.palette();
    }

    static void run() { TestApp16 t{200, 200}; t.loop(); }

protected:

    void draw() override {
        switch (i_) {
            case 0:
                g_.fill(0);
                break;
            case 1:
                g_.fill(1);
                break;
            case 2:
                g_.fill(2);
                break;
            case 3:
                g_.fill(3);
                break;
        }
        i_ = (i_ + 1) % 4;
        Font font(Font::fromROM<assets::font::Iosevka16>());
        g_.text(0,0,font, 5) 
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
        TestApp16::run();
    }
}