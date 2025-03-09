#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/ui/form.h>
#include <rckid/ui/panel.h>
#include <rckid/ui/image.h>
#include <rckid/assets/icons_default_64.h>
#include <rckid/assets/fonts/OpenDyslexic64.h>
#include <rckid/ui/label.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/menu.h>

using namespace rckid;

class DisplayUIApp : public ui::App {
public:

    DisplayUIApp(): ui::App{320, 240} {
        ui::Panel * p1 = new ui::Panel();
        p1->setRect(Rect::XYWH(20, 20, 20, 20));
        p1->setBg(ColorRGB{255, 0, 0});
        ui::Panel * p2 = new ui::Panel();
        p2->setRect(Rect::XYWH(-25, -25, 50, 50));
        p2->setBg(ColorRGB{0, 255, 0});
        //ui::Image * img = new ui::Image{Bitmap<16>{PNG::fromBuffer(assets::icons_default_64::game_controller)}};
        //img->setRect(Rect::XYWH(100, 100, 128, 128));
        //img->setHAlign(HAlign::Center);
        //img->setVAlign(VAlign::Center);
        //img->setRepeat(true);
        ui::Label * l = new ui::Label{0, 50, "Hello world"};
        l->setFont(Font::fromROM<assets::OpenDyslexic64>());
        l->setColor(ColorRGB{255, 255, 255});
        l->setWidth(320);
        l->setHeight(50);
        ui::Menu * m = new ui::Menu{
            new ui::Menu::ActionItem{"Action 1", assets::icons_default_64::animal_1, []() {  }},
            new ui::Menu::ActionItem{"Action 2", assets::icons_default_64::animal_2, []() {  }},
            new ui::Menu::ActionItem{"Action 3", assets::icons_default_64::animal_3, []() {  }},
        };
        c_ = new ui::CarouselMenu{m};
        c_->setRect(Rect::XYWH(0, 160, 320, 80));
        //c->set(ui::Image{Bitmap<16>{PNG::fromBuffer(assets::icons_default_64::animal_2)}}, "Animal 2");
        c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
        //c->moveLeft(ui::Image{Bitmap<16>{PNG::fromBuffer(assets::icons_default_64::animal_1)}}, "Animal 1");
        g_.add(p1);
        g_.add(p2);
        //g_.add(img);
        g_.add(l);
        g_.add(c_);
        g_.setRect(Rect::WH(320, 240));
    }

    void update() override {
        ui::App::update();
        c_->processEvents();
    }

    ui::CarouselMenu * c_;

}; // DisplayUIApp

int main() {
    initialize();
    while (true) {
        auto app = DisplayUIApp{};
        app.run();
    }
}

