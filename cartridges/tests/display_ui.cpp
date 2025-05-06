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
#include <rckid/audio/tone.h>
#include <rckid/apps/MainMenu.h>

#include <rckid/ui/header.h>

#include <rckid/apps/dialogs/TextDialog.h>
#include <rckid/apps/dialogs/FileDialog.h>


using namespace rckid;

class DisplayUIApp : public ui::App<void> {
public:

    DisplayUIApp(): ui::App<void>{320, 240} {
        ui::Panel * p1 = new ui::Panel();
        p1->setRect(Rect::XYWH(20, 20, 20, 20));
        p1->setBg(ColorRGB{255, 0, 0});
        ui::Panel * p2 = new ui::Panel();
        p2->setRect(Rect::XYWH(-25, -25, 50, 50));
        p2->setBg(ColorRGB{0, 255, 0});
        ui::Image * img = new ui::Image{Bitmap<ColorRGB>{PNG::fromBuffer(assets::icons_default_64::game_controller)}};
        img->setRect(Rect::WH(320, 240));
        //img->setHAlign(HAlign::Center);
        //img->setVAlign(VAlign::Center);
        img->setRepeat(true);
        ui::Label * l = new ui::Label{0, 50, "Hello world"};
        l->setFont(Font::fromROM<assets::OpenDyslexic64>());
        l->setColor(ColorRGB{255, 255, 255});
        l->setWidth(320);
        l->setHeight(50);
        c_ = new ui::CarouselMenu{};
        c_->setRect(Rect::XYWH(0, 160, 320, 80));
        //c->set(ui::Image{Bitmap<16>{PNG::fromBuffer(assets::icons_default_64::animal_2)}}, "Animal 2");
        c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
        //c->moveLeft(ui::Image{Bitmap<16>{PNG::fromBuffer(assets::icons_default_64::animal_1)}}, "Animal 1");
        g_.add(p1);
        g_.add(p2);
        g_.add(img);
        g_.add(l);
        g_.add(c_);
        g_.setRect(Rect::WH(320, 240));
    }


    void update() override {
        ui::App<void>::update();
        c_->processEvents();
        if (btnPressed(Btn::A)) {
            t_.on(440);
        } else if (btnPressed(Btn::B)) {
            t_.off();
        }
    }

    void focus() override {
        ui::App<void>::focus();
        t_.setWaveform(Waveform::Sine());
        t_.setSampleRate(44100);
        audioPlay(buf_, 44100, [this](int16_t * buf, uint32_t size) {
            return t_.generateInto(buf, size);
        });
    }

    void blur() override {
        audioStop();
        t_.off();
        ui::App<void>::blur();
    }

    ui::CarouselMenu * c_;

    Tone t_;
    DoubleBuffer<int16_t> buf_{2048};

}; // DisplayUIApp

ui::Menu * mainMenuGenerator(void*) {
    return new ui::Menu{
        new ui::Menu::SubmenuItem{"Games", assets::icons_default_64::game_controller, ui::Menu::Generator{[](void*) { return new ui::Menu{
            new ui::Menu::ActionItem{"Tetris", assets::icons_default_64::tetris, ui::Menu::Action{App::run<TextDialog>}},
            new ui::Menu::ActionItem{"Game 1", assets::icons_default_64::animal, ui::Menu::Action{}},
            new ui::Menu::ActionItem{"Game 2", assets::icons_default_64::animal_1, ui::Menu::Action{}},
            new ui::Menu::ActionItem{"Game 3", assets::icons_default_64::animal_2, ui::Menu::Action{}},
        }; }}},
        new ui::Menu::ActionItem{"Music", assets::icons_default_64::music, ui::Menu::Action{}},
        new ui::Menu::ActionItem{"Messages", assets::icons_default_64::chat, ui::Menu::Action{}},
        new ui::Menu::ActionItem{"WalkieTalkie", assets::icons_default_64::baby_monitor, ui::Menu::Action{}},
        new ui::Menu::ActionItem{"Birthdays", assets::icons_default_64::birthday_cake, ui::Menu::Action{}},
        new ui::Menu::ActionItem{"Clock", assets::icons_default_64::alarm_clock, ui::Menu::Action{}},
        new ui::Menu::ActionItem{"Remote", assets::icons_default_64::rc_car, ui::Menu::Action{}},
        new ui::Menu::ActionItem{"Recorder", assets::icons_default_64::microphone, ui::Menu::Action{}},
        new ui::Menu::ActionItem{"Files", assets::icons_default_64::folder, ui::Menu::Action{App::run<FileDialog>}},
        new ui::Menu::ActionItem{"Composer", assets::icons_default_64::music_1, ui::Menu::Action{}},
        new ui::Menu::ActionItem{"Drawing", assets::icons_default_64::paint_palette, ui::Menu::Action{}},
    };
}




int main() {
    initialize();
    while (true) {
        LOG(LL_INFO, "Free memory: " << memoryFree() / 1024);
        //auto app = DisplayUIApp{};
        auto app = MainMenu::run(mainMenuGenerator);
        if (app.has_value()) {
            app.value()();
        }
/*
        auto menu = MainMenu{mainMenuGenerator};
        auto app = menu.run();
        if (app.has_value()) {
            auto text = app.value();
            auto dialog = TextDialog::run("You selected: " + text);
            if (dialog.has_value()) {
                LOG(LL_INFO, "Dialog result: " << dialog.value());
            } else {
                LOG(LL_INFO, "Dialog cancelled");
            }
        } else {
            LOG(LL_INFO, "Menu cancelled");
        }
            app = app.value();
        else
            break;
        ASSERT(app.has_value());
        ui::Menu::ActionItem app = menu.run();
        app.action()();
        */
    }
}

