#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/ui/form.h>
#include <rckid/ui/panel.h>
#include <rckid/ui/image.h>
#include <rckid/assets/icons_default_64.h>
#include <rckid/ui/label.h>

using namespace rckid;

class DisplayUIApp : public App<ui::Form> {
public:

    DisplayUIApp(): App{320, 240} {
        ui::Panel * p1 = new ui::Panel();
        p1->setRect(Rect::XYWH(20, 20, 20, 20));
        p1->setBg(ColorRGB{255, 0, 0});
        ui::Panel * p2 = new ui::Panel();
        p2->setRect(Rect::XYWH(-25, -25, 50, 50));
        p2->setBg(ColorRGB{0, 255, 0});
        ui::Image<16> * img = new ui::Image<16>{PNG::fromBuffer(assets::icons_default_64::game_controller)};
        img->setRect(Rect::XYWH(100, 100, 128, 128));
        img->setHAlign(HAlign::Center);
        img->setVAlign(VAlign::Center);
        img->setRepeat(true);
        ui::Label * l = new ui::Label{0, 50, "Hello world"};
        l->setColor(ColorRGB{255, 255, 255});
        l->setWidth(320);
        l->setHeight(50);
        g_.add(p1);
        g_.add(p2);
        g_.add(img);
        g_.add(l);
        g_.setRect(Rect::WH(320, 240));
    }

    static void run() { DisplayUIApp t; t.loop(); }

}; // DisplayUIApp

int main() {
    initialize();
    while (true) {
        DisplayUIApp::run();
    }
}

