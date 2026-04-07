
#include <rckid/apps/music_player.h>
#include <rckid/apps/steps.h>
#include <rckid/apps/data_sync.h>
#include <rckid/apps/file_browser.h>
#include <rckid/apps/friends.h>
#include <rckid/apps/messages.h>
#include <rckid/apps/about.h>

#include <rckid/apps/games/blocks.h>

#include <rckid/apps/launcher.h>

// TODO this will not be here eventyually
#include <rckid/game/engine.h>
#include <rckid/apps/cat_chase.h>

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
                << ui::MenuItem{"Messages", assets::icons_64::poo, []() {
                    App::run<Messages>();
                }}
                << ui::MenuItem::Generator("Utilities", assets::icons_64::configuration, utilitiesMenuGenerator)
                << ui::MenuItem::Generator("Settings", assets::icons_64::poo, settingsMenuGenerator);
            return result;
        };
    }

    unique_ptr<ui::Menu> gamesMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
            (*result)
                << ui::MenuItem{"Game Engine", assets::icons_64::gameboy, []() {
                    App::run<CatChase>();
                }};
        return result;
    }

    unique_ptr<ui::Menu> utilitiesMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem{"Steps", assets::icons_64::footprint, []() {
                App::run<Steps>();
            }}
            << ui::MenuItem{"Data Sync", assets::icons_64::pen_drive, []() {
                App::run<DataSync>();
            }}
            << ui::MenuItem("File Browser", assets::icons_64::folder, []() {
                App::run<FileBrowser>();
            });

        return result;
    }

    unique_ptr<ui::Menu> settingsMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem{"About", assets::icons_64::info, []() {
                App::run<About>();
            }};
        return result;
    }

} // namespace rckid