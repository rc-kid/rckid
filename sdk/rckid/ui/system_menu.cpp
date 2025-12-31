#include "../app.h"

#include "../apps/games/Tetris.h"
#include "../apps/games/Checkers.h"
#include "../apps/utils/Flashlight.h"
#include "../apps/utils/Clock.h"
#include "../apps/utils/Stopwatch.h"
#include "../apps/utils/Timer.h"
#include "../apps/utils/PiggyBank.h"
#include "../apps/utils/About.h"
#include "../apps/utils/Steps.h"
#include "../apps/DataSync.h"

#include "../apps/MainMenu.h"
#include "../apps/Friends.h"
#include "../apps/Recorder.h"
#include "../apps/Drawing.h"
#include "../apps/MusicPlayer.h"
#include "../apps/FMRadio.h"
#include "../apps/Messages.h"

#include "../apps/dialogs/PinDialog.h"
#include "../apps/dialogs/InfoDialog.h"
#include "../apps/dialogs/Slider.h"

#include "../apps/devel/HardwareStatus.h"
#include "../apps/devel/SDTest.h"
#include "../apps/devel/WiFiScan.h"

#include "../utils/ini.h"

#include "system_menu.h"

namespace rckid::ui {

    ui::ActionMenu * styleMenuGenerator();
    ui::ActionMenu * parentMenuGenerator();

    void runSystemMenu(ui::ActionMenu::MenuGenerator menuGenerator) {
        while (true) {
            LOG(LL_INFO, "Free memory: " << memoryFree() / 1024);
            auto app = App::run<MainMenu>(menuGenerator);
            LOG(LL_INFO, "MainMenu done");
            yield();
            LOG(LL_INFO, "Free memory: " << memoryFree() / 1024);
            yield();
            ASSERT((app.has_value()));
            {
                RAMHeap::LeakGuard g_;
                app.value()();
            }
        }
    }

    std::unordered_set<String> getBlacklistedApps() {
        std::unordered_set<String> result;
        ini::Reader ini{fs::fileRead("/apps.ini")};
        if (! ini.eof()) {
            while (auto section = ini.nextSection()) {
                if (section.has_value() && section.value() == "blacklist") {
                    // ignore the keys, just collect the values
                    while (auto pair = ini.nextValue())
                        result.insert(pair->second);
                }
            }
        }
        return result;
    }

    void addAppToMenu(ui::ActionMenu * menu, String const & appName, Icon const & icon, std::function<void()> launchFunc, std::unordered_set<String> const & blacklist) {
        if (blacklist.find(appName) == blacklist.end()) {
            menu->add(ui::ActionMenu::Item(appName, icon, std::move(launchFunc)));
        }
    }

    ui::ActionMenu * mainMenuGenerator() {
        std::unordered_set<String> blacklist{getBlacklistedApps()};
        auto result = new ui::ActionMenu{};
        result->add(ui::ActionMenu::Generator("Games", assets::icons_64::game_controller, gamesMenuGenerator));
        addAppToMenu(result, "Music", assets::icons_64::music, App::run<MusicPlayer>, blacklist);
        addAppToMenu(result, "Radio", assets::icons_64::radio_cassette, App::run<FMRadio>, blacklist);
        addAppToMenu(result, "Friends", assets::icons_64::birthday_cake, App::run<Friends>, blacklist);
        addAppToMenu(result, "Messages", assets::icons_64::chat, App::run<Messages>, blacklist);
        // TODO should drawing move to its own submenu with other image/asset stuff? 
        addAppToMenu(result, "Drawing", assets::icons_64::paint_palette, App::run<Drawing>, blacklist);
        result->add(ui::ActionMenu::Generator("Audio", assets::icons_64::music_2, audioMenuGenerator));
        result->add(ui::ActionMenu::Generator("Utilities", assets::icons_64::configuration, utilsMenuGenerator));
        result->add(ui::ActionMenu::Generator("Settings", assets::icons_64::settings, settingsMenuGenerator));
        if (debugMode())
            result->add(ui::ActionMenu::Generator("Devel", assets::icons_64::ladybug, develMenuGenerator));
        return result;
    }

    ui::ActionMenu * gamesMenuGenerator() {
        
        ui::ActionMenu * result = new ui::ActionMenu{
            ui::ActionMenu::Item("Tetris", assets::icons_64::tetris, App::run<Tetris>),
            //ui::ActionMenu::Item("Checkers", assets::icons_64::poo, App::run<Checkers>),
        };

        // and return the menu
        return result;
    }

    ui::ActionMenu * utilsMenuGenerator() {
        std::unordered_set<String> blacklist{getBlacklistedApps()};
        auto result = new ui::ActionMenu{};
        addAppToMenu(result, "Flashlight", assets::icons_64::flashlight, App::run<Flashlight>, blacklist);
        addAppToMenu(result, "Clock", assets::icons_64::alarm_clock, App::run<Clock>, blacklist);
        addAppToMenu(result, "Stopwatch", assets::icons_64::chronometer, App::run<Stopwatch>, blacklist);
        //addAppToMenu(result, "Steps", assets::icons_64::footprint, App::run<Steps>, blacklist);
        // TODO add piggy bank
        //addAppToMenu(result, "Timer", assets::icons_64::hourglass, App::run<Timer>, blacklist);
        addAppToMenu(result, "Piggy Bank", assets::icons_64::piggy_bank, App::run<PiggyBank>, blacklist);
        // TODO add proper file manager
        //addAppToMenu(result, "Files", assets::icons_64::folder, App::run<FileDialog>, blacklist);
        addAppToMenu(result, "Data Sync", assets::icons_64::pen_drive, App::run<DataSync>, blacklist);
        return result;
    }

    ui::ActionMenu * commsMenuGenerator() {
        std::unordered_set<String> blacklist{getBlacklistedApps()};
        auto result = new ui::ActionMenu{};
        addAppToMenu(result, "Friends", assets::icons_64::birthday_cake, App::run<Friends>, blacklist); 
        addAppToMenu(result, "Messages", assets::icons_64::chat, App::run<Messages>, blacklist);
        // TODO NRF or LoRa cartridges, libopus encoding/decoding, recording 
        //addAppToMenu(result, "WalkieTalkie", assets::icons_64::baby_monitor, nullptr, blacklist);
        return result;
    }

    ui::ActionMenu * audioMenuGenerator() {
        std::unordered_set<String> blacklist{getBlacklistedApps()};
        auto result = new ui::ActionMenu{};
        // TODO recorder not ready yet, microphone is doing silly things
        addAppToMenu(result, "Recorder", assets::icons_64::microphone, App::run<Recorder>, blacklist);
        // TODO composer app
        // addAppToMenu(result, "Composer", assets::icons_64::music_2, nullptr, blacklist);
        return result;
    }

    ui::ActionMenu * imagesMenuGenerator() {
        return new ui::ActionMenu{
            // TODO enable when ready
            //ui::ActionMenu::Item("Drawing", assets::icons_64::paint_palette, App::run<Drawing>),
            // TODO browser for images alone
        };
    }

    ui::ActionMenu * settingsMenuGenerator() {
        return new ui::ActionMenu{
            ui::ActionMenu::Item("Myself", Myself::contact().image, []() {
                auto x = App::run<Friends::ContactViewer>(Myself::contact());
                if (x.has_value() && x.value())
                    Myself::save();
            }),
            ui::ActionMenu::Generator("Style", assets::icons_64::paint_palette, styleMenuGenerator),
            ui::ActionMenu::Generator("Lights", assets::icons_64::brightness_1, []() {
                return new ui::ActionMenu{
                    ui::ActionMenu::Item("Off", assets::icons_64::turn_off, [](){
                        ui::Style::setRgbStyle(ui::RGBStyle::Off);
                        ui::Style::save();
                    }),
                    ui::ActionMenu::Item("Rainbow", assets::icons_64::rainbow, [](){
                        ui::Style::setRgbStyle(ui::RGBStyle::Rainbow);
                        ui::Style::save();
                    }),
                    ui::ActionMenu::Item("Rainbow Wave", assets::icons_64::rainbow, [](){
                        ui::Style::setRgbStyle(ui::RGBStyle::RainbowWave);
                        ui::Style::save();
                    }),
                    ui::ActionMenu::Item("Breathe", assets::icons_64::light, [](){
                        auto c = App::run<ColorPicker>(ui::Style::rgbColor());
                        if (c.has_value()) {
                            ui::Style::setRgbColor(c.value());
                            ui::Style::setRgbStyle(ui::RGBStyle::Breathe);
                        ui::Style::save();
                        }
                    }),
                    ui::ActionMenu::Item("Breathe Wave", assets::icons_64::light, [](){
                        auto c = App::run<ColorPicker>(ui::Style::rgbColor());
                        if (c.has_value()) {
                            ui::Style::setRgbColor(c.value());
                            ui::Style::setRgbStyle(ui::RGBStyle::BreatheWave);
                        ui::Style::save();
                        }
                    }),
                    ui::ActionMenu::Item("Solid", assets::icons_64::light, [](){
                        auto c = App::run<ColorPicker>(ui::Style::rgbColor());
                        if (c.has_value()) {
                            ui::Style::setRgbColor(c.value());
                            ui::Style::setRgbStyle(ui::RGBStyle::Solid);
                        ui::Style::save();
                        }
                    }),
                    ui::ActionMenu::Item("Key", assets::icons_64::down_arrow, [](){
                        auto c = App::run<ColorPicker>(ui::Style::rgbColor());
                        if (c.has_value()) {
                            ui::Style::setRgbColor(c.value());
                            ui::Style::setRgbStyle(ui::RGBStyle::Key);
                            ui::Style::save();
                        }
                    }),
                    ui::ActionMenu::Item("Rainbow Key", assets::icons_64::rainbow, [](){
                        ui::Style::setRgbStyle(ui::RGBStyle::RainbowKey);
                        ui::Style::save();
                    }),
                    ui::ActionMenu::Item("Brightness", assets::icons_64::brightness, [](){
                        Slider s{assets::icons_64::brightness_1, "Brightness", 0, 31, ui::Style::rgbBrightness() >> 1, [](uint32_t value) {
                            ui::Style::setRgbBrightness((value << 1)  + (value & 1)); 
                        }};
                        // TODO we do not support generic animations here, fix when we have UI overhaul
                        //s.setAnimation(c_.iconPosition(), c_.textPosition());
                        s.loop();
                        ui::Style::save();
                    }),
                };
            }),
            ui::ActionMenu::Item("Rumbler", assets::icons_64::vibration, [](){
                Slider s{assets::icons_64::vibration, "Rumbler", 0, 7, ui::Style::rumblerKeyPressIntensity() >> 5, [](uint32_t value) {
                    ui::Style::setRumblerKeyPressIntensity((value << 5) + (value << 2) + (value & 0x03)); 
                }};
                // TODO we do not support generic animations here, fix when we have UI overhaul
                //s.setAnimation(c_.iconPosition(), c_.textPosition());
                s.loop();
                ui::Style::save();
            }),
            ui::ActionMenu::Item("Pin", assets::icons_64::lock, [](){
                if (pinCurrent() != 0xffff) {
                    auto x = App::run<PinDialog>("Current pin");
                    if (! x.has_value())
                        return;
                    if (x.value() != pinCurrent()) {
                        InfoDialog::error("Incorrect PIN", "The PIN you entered is incorrect.");
                        return;
                    }
                }
                auto x = App::run<PinDialog>("New pin");
                if (! x.has_value())
                    return;
                uint16_t newPin = x.value();
                x = App::run<PinDialog>("New pin again");
                if (! x.has_value())
                    return;
                if (x.value() != newPin) {
                    InfoDialog::error("PIN mismatch", "The new PIN entries do not match.");
                    return;
                }
                pinSet(newPin);
                InfoDialog::success("Done", "PIN changed successfully");
            }),
            ui::ActionMenu::Generator("Parent Mode", assets::icons_64::family, parentMenuGenerator),
            ui::ActionMenu::Item("About", assets::icons_64::info, App::run<About>),
        };
    }

    ui::ActionMenu * develMenuGenerator() {
        return new ui::ActionMenu{
            ui::ActionMenu::Item("Debug Off", assets::icons_64::ladybug, [](){
                rckid::setDebugMode(false);
            }),
            ui::ActionMenu::Item("Debug On", assets::icons_64::ladybug, [](){
                rckid::setDebugMode(true);
            }),
            ui::ActionMenu::Item("HW Status", assets::icons_64::pen_drive, App::run<HardwareStatus>),
            ui::ActionMenu::Item("SD Test", assets::icons_64::pen_drive, App::run<SDTest>),
            ui::ActionMenu::Item("WiFi Scan", assets::icons_64::pen_drive, App::run<WiFiScan>),
        };
    }

    // submenu items

    ui::ActionMenu * styleMenuGenerator() {
        return new ui::ActionMenu{
            ui::ActionMenu::Item("Text", assets::icons_64::color_picker, []() {
                auto c = App::run<ColorPicker>(ui::Style::fg());
                if (c.has_value()) {
                    ui::Style::setFg(c.value());
                    ui::Style::refreshAndSave();
                }
            }),
            ui::ActionMenu::Item("Bg", assets::icons_64::color_picker, [](){
                auto c = App::run<ColorPicker>(ui::Style::bg());
                if (c.has_value()) {
                    ui::Style::setBg(c.value());
                    ui::Style::refreshAndSave();
                }
            }),
            ui::ActionMenu::Item("Accent Text", assets::icons_64::color_picker, []() {
                auto c = App::run<ColorPicker>(ui::Style::accentFg());
                if (c.has_value()) {
                    ui::Style::setAccentFg(c.value());
                    ui::Style::refreshAndSave();
                }
            }),
            ui::ActionMenu::Item("Accent Bg", assets::icons_64::color_picker, [](){
                auto c = App::run<ColorPicker>(ui::Style::accentBg());
                if (c.has_value()) {
                    ui::Style::setAccentBg(c.value());
                    ui::Style::refreshAndSave();
                }
            }),
            ui::ActionMenu::Item("Background", assets::icons_64::picture, [](){
                auto icon = App::run<FileDialog>("Select image", "/files/images");
                if (icon.has_value()) {
                    ui::Style::setBackground(Icon{icon.value().c_str()});
                    ui::Style::refreshAndSave();
                }
            }),
        };
    }

    ui::ActionMenu * parentMenuGenerator() {
        if (parentMode()) {
            return new ui::ActionMenu{
                ui::ActionMenu::Item("Leave", assets::icons_64::logout, [](){
                    rckid::setParentMode(false);
                }),
                ui::ActionMenu::Item("Top-Up", assets::icons_64::up_arrow, [](){
                    auto result = App::run<TimeDialog>("Top up time", TinyTime{0, 30});
                    if (result.has_value()) {
                        budgetSet(budget() + (result.value().hour() * 60 + result.value().minute()) * 60);
                    }
                }),
                ui::ActionMenu::Item("Daily Budget", assets::icons_64::heart, [](){
                    auto result = App::run<TimeDialog>("Daily Budget", TinyTime{budgetDaily()});
                    if (result.has_value()) {
                        // store the daily budget in both *avr* and in the ini file on the SD card
                        budgetDailySet((result.value().hour() * 60 + result.value().minute()) * 60);
                        Myself::save();
                    }
                }),
                ui::ActionMenu::Item("Clear Pin", assets::icons_64::lock, [](){
                    pinSet(0xffff);
                }),
                ui::ActionMenu::Item("Set password", assets::icons_64::lock, [](){
                    auto x = App::run<TextDialog>("New password");
                    if (! x.has_value())
                        return;
                    String newPassword = x.value();
                    x = App::run<TextDialog>("New password again");
                    if (! x.has_value())
                        return;
                    if (x.value() != newPassword) {
                        InfoDialog::error("Incorrect Password", "The passwords you entered do not match.");
                        return;
                    }
                    Myself::setParentPassword(newPassword);
                }),
            };
        } else {
            return new ui::ActionMenu{
                ui::ActionMenu::Item("Enter", assets::icons_64::family, [](){
                    if (! Myself::parentPassword().empty()) {
                        auto x = App::run<TextDialog>("Parent Password");
                        if (! x.has_value())
                            return;
                        if (x.value() != Myself::parentPassword()) {
                            InfoDialog::error("Incorrect Password", "The password you entered is incorrect.");
                            // TODO should there be budget penalty? 
                            return;
                        }
                    }
                    rckid::setParentMode(true);
                }),
            };
        }
    }



} // namespace rckid::ui