
#include <rckid/apps/music_player.h>
#include <rckid/apps/steps.h>
#include <rckid/apps/data_sync.h>
#include <rckid/apps/file_browser.h>
#include <rckid/apps/friends.h>

#include <rckid/apps/launcher.h>

namespace rckid {
   
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
    }

    unique_ptr<ui::Menu> gamesMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        return result;
    }

    unique_ptr<ui::Menu> utilitiesMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem{"Steps", assets::icons_64::footprint, []() {
                Steps{}.run();
            }}
            << ui::MenuItem{"Data Sync", assets::icons_64::pen_drive, []() {
                DataSync{}.run();
            }}
            << ui::MenuItem("File Browser", assets::icons_64::folder, []() {
                FileBrowser{}.run();
            });

        return result;
    }

} // namespace rckid