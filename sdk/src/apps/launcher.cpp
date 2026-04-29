
#include <rckid/ui/style.h>

#include <rckid/apps/music_player.h>
#include <rckid/apps/friends.h>
#include <rckid/apps/messages.h>

#include <rckid/apps/utils/about.h>
#include <rckid/apps/utils/steps.h>
#include <rckid/apps/utils/data_sync.h>
#include <rckid/apps/utils/file_browser.h>
#include <rckid/apps/utils/clock.h>
#include <rckid/apps/utils/stopwatch.h>
#include <rckid/apps/utils/flashlight.h>
#include <rckid/apps/utils/piggy_bank.h>

#include <rckid/apps/dialogs/file_dialog.h>
#include <rckid/apps/dialogs/color_dialog.h>

#include <rckid/apps/games/blocks.h>

#include <rckid/apps/launcher.h>


// TODO this will not be here eventually
#include <rckid/game/engine.h>
#include <rckid/apps/games/cat_chase.h>

namespace rckid {

    ui::MenuItem::GeneratorEvent mainMenuGenerator(MainMenuOptions options) {
        return [options]() {
            auto result = std::make_unique<ui::Menu>();
            (*result)
                << ui::MenuItem::Generator("Games", assets::icons_64::game_controller, [extend = options.gamesExtender]() {
                    auto gamesMenu = gamesMenuGenerator();
                    if (extend != nullptr)
                        return extend(std::move(gamesMenu));
                    else 
                        return gamesMenu;
                })
                << ui::MenuItem{"Music", assets::icons_64::music, []() {
                    App::run<MusicPlayer>();
                }}
                << ui::MenuItem{"Friends", assets::icons_64::birthday_cake, []() {
                    App::run<Friends>();
                }}
                << ui::MenuItem{"Messages", assets::icons_64::chat, []() {
                    App::run<Messages>();
                }}
                << ui::MenuItem::Generator("Utilities", assets::icons_64::configuration, utilitiesMenuGenerator)
                << ui::MenuItem::Generator("Settings", assets::icons_64::settings, settingsMenuGenerator);
            return result;
        };
    }

    unique_ptr<ui::Menu> gamesMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
            (*result)
                << ui::MenuItem{"Game Engine", assets::icons_64::gameboy, []() {
                    App::run<CatChase>();
                }}
                << ui::MenuItem{"Blocks", assets::icons_64::tetris, []() {
                    App::run<Blocks>();
                }};
        return result;
    }

    unique_ptr<ui::Menu> utilitiesMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem{"Clock", assets::icons_64::alarm_clock, []() {
                App::run<Clock>();
            }}
            << ui::MenuItem{"Stopwatch", assets::icons_64::chronometer, []() {
                App::run<Stopwatch>();
            }}
            << ui::MenuItem{"Flashlight", assets::icons_64::flashlight, []() {
                App::run<Flashlight>();
            }}
            << ui::MenuItem{"Steps", assets::icons_64::footprint, []() {
                App::run<Steps>();
            }}
            << ui::MenuItem{"Piggy Bank", assets::icons_64::piggy_bank, []() {
                App::run<PiggyBank>();
            }}
            << ui::MenuItem{"Data Sync", assets::icons_64::pen_drive, []() {
                App::run<DataSync>();
            }}
            << ui::MenuItem("File Browser", assets::icons_64::folder, []() {
                App::run<FileBrowser>();
            });

        return result;
    }

    unique_ptr<ui::Menu> styleSettingsMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem::Generator("Background", assets::icons_64::picture, [](){
                auto result = std::make_unique<ui::Menu>();
                (*result)
                    << ui::MenuItem{"Image", assets::icons_64::picture, []() {
                        auto path = App::run<FileDialog>("/files/images/backgrounds");
                        if (path) {
                            ui::Style & style = ui::Style::defaultStyle();
                            style.setBackgroundImage(ImageSource{path.value()});
                            // force repaint
                            Launcher::updateStyle(style);
                            ui::Style::saveDefaultStyle();
                        }
                    }};
                return result;
            })
            << ui::MenuItem::Generator("Colors", assets::icons_64::light, [](){
                auto result = std::make_unique<ui::Menu>();
                (*result)
                    << ui::MenuItem{"Text", assets::icons_64::light, []() {
                        ui::Style & style = ui::Style::defaultStyle();
                        auto color = App::run<ColorDialog>(style.defaultFg());
                        if (color) {
                            style.setDefaultFg(color.value());
                            Launcher::updateStyle(style);
                            ui::Style::saveDefaultStyle();
                        }
                    }}
                    << ui::MenuItem{"Background", assets::icons_64::light, []() {
                        ui::Style & style = ui::Style::defaultStyle();
                        auto color = App::run<ColorDialog>(style.defaultBg());
                        if (color) {
                            style.setDefaultBg(color.value());
                            Launcher::updateStyle(style);
                            ui::Style::saveDefaultStyle();
                        }
                    }}
                    << ui::MenuItem{"Accent Text", assets::icons_64::light, []() {
                        ui::Style & style = ui::Style::defaultStyle();
                        auto color = App::run<ColorDialog>(style.accentFg());
                        if (color) {
                            style.setAccentFg(color.value());
                            Launcher::updateStyle(style);
                            ui::Style::saveDefaultStyle();
                        }
                    }}
                    << ui::MenuItem{"Accent Bg", assets::icons_64::light, []() {
                        ui::Style & style = ui::Style::defaultStyle();
                        auto color = App::run<ColorDialog>(style.accentBg());
                        if (color) {
                            style.setAccentBg(color.value());
                            Launcher::updateStyle(style);
                            ui::Style::saveDefaultStyle();
                        }
                    }};
                return result;
            })
            << ui::MenuItem::Generator("Keys", assets::icons_64::poo, [](){
                auto result = std::make_unique<ui::Menu>();
                /* TODO should this really be user controllable
                (*result)
                    << ui::MenuItem{"Repeat", assets::icons_64::picture, []() {
                        UNIMPLEMENTED;
                    }};
                */
                return result;
            });

        return result;
    }

    unique_ptr<ui::Menu> settingsMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem::Generator("Style", assets::icons_64::paint_palette, styleSettingsMenuGenerator)
            << ui::MenuItem{"About", assets::icons_64::info, []() {
                App::run<About>();
            }};
        return result;
    }

    void Launcher::updateStyle(ui::Style & style) {
        // only expected to be called from the launcher
        if (instance_ == nullptr)
            return;
        instance_->root_.setBackgroundImage(style);
    }


} // namespace rckid