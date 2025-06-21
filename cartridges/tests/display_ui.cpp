#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/ui/form.h>
#include <rckid/ui/panel.h>
#include <rckid/ui/image.h>
#include <rckid/assets/icons_64.h>
#include <rckid/assets/fonts/OpenDyslexic64.h>
#include <rckid/ui/label.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/menu.h>
#include <rckid/audio/tone.h>
#include <rckid/apps/MainMenu.h>

#include <rckid/ui/header.h>

#include <rckid/apps/dialogs/TextDialog.h>
#include <rckid/apps/dialogs/FileDialog.h>
#include <rckid/apps/dialogs/InfoDialog.h>
#include <rckid/apps/dialogs/PopupMenu.h>
#include <rckid/apps/AudioPlayer.h>
#include <rckid/apps/Friends.h>
#include <rckid/apps/Clock.h>

#include <gbcemu/gbcemu.h>
#include <gbcemu/gamepak.h>


using namespace rckid;

class DisplayUIApp : public ui::App<void> {
public:

    DisplayUIApp(): ui::App<void>{320, 240} {
        ui::Panel * p1 = g_.addChild(new ui::Panel());
        p1->setRect(Rect::XYWH(20, 20, 20, 20));
        p1->setBg(ColorRGB{255, 0, 0});
        ui::Panel * p2 = g_.addChild(new ui::Panel());
        p2->setRect(Rect::XYWH(-25, -25, 50, 50));
        p2->setBg(ColorRGB{0, 255, 0});
        ui::Image * img = g_.addChild(new ui::Image{Bitmap2{PNG::fromBuffer(assets::icons_64::game_controller)}});
        img->setRect(Rect::WH(320, 240));
        //img->setHAlign(HAlign::Center);
        //img->setVAlign(VAlign::Center);
        img->setRepeat(true);
        ui::Label * l = g_.addChild(new ui::Label{0, 50, "Hello world"});
        l->setFont(Font::fromROM<assets::OpenDyslexic64>());
        l->setColor(ColorRGB{255, 255, 255});
        l->setWidth(320);
        l->setHeight(50);
        c_ = g_.addChild(new ui::CarouselMenu{});
        c_->setRect(Rect::XYWH(0, 160, 320, 80));
        //c->set(ui::Image{Bitmap<16>{PNG::fromBuffer(assets::icons_64::animal_2)}}, "Animal 2");
        c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
        //c->moveLeft(ui::Image{Bitmap<16>{PNG::fromBuffer(assets::icons_64::animal_1)}}, "Animal 1");
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

/** Game menu generator. 
 
    Returns the games available in the system. The games are either statically known, such as built-in games, or cartridge stored ROMs, or can be dynamic, by looking at roms in the games folder.
 */
ui::Menu * gamesGenerator() {
    
    ui::Menu * result = new ui::Menu{
        MainMenu::Action("Tetris", assets::icons_64::tetris, App::run<TextDialog>),
    };
    // now get all the menus 
    fs::Folder games = fs::folderRead("/games");
    for (auto & entry : games) {
        if (entry.isFile() && (fs::ext(entry.name()) == ".gb")) {
            LOG(LL_INFO, "Found game: " << entry.name());
            result->add(MainMenu::GameLauncher(fs::stem(entry.name()), assets::icons_64::gameboy, entry.name()));
        }
    }
    return result;
}

ui::Menu * utilsMenuGenerator() {
    return new ui::Menu{
        MainMenu::Action("Clock", assets::icons_64::alarm_clock, App::run<Clock>),
        MainMenu::Action("Stopwatch", assets::icons_64::poo, nullptr),
        MainMenu::Action("Timer", assets::icons_64::poo, nullptr),
        MainMenu::Action("Files", assets::icons_64::folder, App::run<FileDialog>),
    };
}

ui::Menu * commsMenuGenerator() {
    return new ui::Menu{
        MainMenu::Action("Messages", assets::icons_64::chat, nullptr),
        MainMenu::Action("WalkieTalkie", assets::icons_64::baby_monitor, nullptr),
        MainMenu::Action("Friends", assets::icons_64::birthday_cake, App::run<Friends>),
    };
}

ui::Menu * audioMenuGenerator() {
    return new ui::Menu{
        MainMenu::Action("Recorder", assets::icons_64::microphone, nullptr),
        MainMenu::Action("Composer", assets::icons_64::music_1, nullptr),
        // TODO browser for audio files alone
    };
}

ui::Menu * imagesMenuGenerator() {
    return new ui::Menu{
        MainMenu::Action("Drawing", assets::icons_64::paint_palette, nullptr),
        // TODO browser for images alone
    };
}

ui::Menu * mainMenuGenerator() {
    return new ui::Menu{
        MainMenu::Submenu("Games", assets::icons_64::game_controller, gamesGenerator),
        MainMenu::Action("Music", assets::icons_64::music, App::run<AudioPlayer>),
        MainMenu::Submenu("Comms", assets::icons_64::chat, commsMenuGenerator),
        MainMenu::Submenu("Audio", assets::icons_64::poo, audioMenuGenerator),
        MainMenu::Submenu("Images", assets::icons_64::poo, imagesMenuGenerator),
        MainMenu::Action("Remote", assets::icons_64::rc_car, nullptr),
        MainMenu::Submenu("Utilities", assets::icons_64::poo, utilsMenuGenerator),
    };
}

int main() {
    initialize();

    //PNG png{PNG::fromStream(fs::fileRead(STR("files/images/backgrounds/wish16.png")))};
    //LOG(LL_INFO, "PNG loaded: " << png.width() << "x" << png.height() << ", bpp: " << png.bpp());
    while (true) {
        LOG(LL_INFO, "Free memory: " << memoryFree() / 1024);
        //auto app = DisplayUIApp{};
        auto app = MainMenu::run(mainMenuGenerator);
        LOG(LL_INFO, "MainMenu done");
        yield();
        LOG(LL_INFO, "Free memory: " << memoryFree() / 1024);
        yield();
        if (app.has_value()) {
            std::visit(overload{
                [](ui::Menu::Generator const & gen) {
                    UNREACHABLE; // Should not see menu generator here
                },
                [](ui::Menu::Action const & action) {
                    LOG(LL_INFO, "Running app...");
                    yield();
                    action();
                },
                [](MainMenuGameLauncher const & gl) {
                    // TODO some nicer way to run the game? 
                    NewArenaGuard g{};
                    LOG(LL_INFO, "running game: " << gl.file);
                    auto app = gbcemu::GBCEmu{};
                    app.loadCartridge(new gbcemu::CachedGamePak{fs::fileRead(STR("/games/" << gl.file))});
                    app.run();
                }
            }, app.value());
        } else {
            InfoDialog::error("Empty app", "The app you have chosen is empty. Ouch");
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

