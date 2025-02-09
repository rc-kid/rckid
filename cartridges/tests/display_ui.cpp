#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/ui/form.h>
#include <rckid/ui/panel.h>

using namespace rckid;

class DisplayUIApp : public App<ui::Form> {
public:

    DisplayUIApp() {
        ui::Panel * p1 = new ui::Panel();
        p1->setRect(Rect::XYWH(20, 20, 20, 20));
        p1->setBg(ColorRGB{255, 0, 0});
        ui::Panel * p2 = new ui::Panel();
        p2->setRect(Rect::XYWH(-25, -25, 50, 50));
        p2->setBg(ColorRGB{0, 255, 0});
        g_.add(p1);
        g_.add(p2);
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

