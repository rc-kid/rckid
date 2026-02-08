
#include <rckid/apps/music_player.h>
#include <rckid/apps/steps.h>

#include <rckid/apps/launcher.h>

namespace rckid {
   
    unique_ptr<ui::Menu> mainMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem::Generator("Games", assets::icons_64::game_controller, gamesMenuGenerator)
            << ui::MenuItem{"Music", assets::icons_64::music, []() {
                MusicPlayer{}.run();
            }}
            << ui::MenuItem::Generator("Utilities", assets::icons_64::configuration, utilitiesMenuGenerator)
            << ui::MenuItem{"Steps Two", assets::icons_64::footprint, []() {
                Steps{}.run();
            }};
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
            }};
        return result;
    }
    

} // namespace rckid