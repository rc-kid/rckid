
#include <rckid/apps/music_player.h>
#include <rckid/apps/steps.h>
#include <rckid/apps/data_sync.h>
#include <rckid/apps/file_browser.h>
#include <rckid/apps/friends.h>

#include <rckid/apps/launcher.h>

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
                << ui::MenuItem::Generator("Utilities", assets::icons_64::configuration, utilitiesMenuGenerator);
            return result;
        };
    }
   
    /*
    unique_ptr<ui::Menu> mainMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem::Generator("Games", assets::icons_64::game_controller, gamesMenuGenerator)
            << ui::MenuItem{"Music", assets::icons_64::music, []() {
                MusicPlayer{}.run();
            }}
            << ui::MenuItem{"Friends", assets::icons_64::birthday_cake, []() {
                Friends{}.run();
            }}
            << ui::MenuItem::Generator("Utilities", assets::icons_64::configuration, utilitiesMenuGenerator);
        return result;
    }*/

    unique_ptr<ui::Menu> gamesMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
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

} // namespace rckid