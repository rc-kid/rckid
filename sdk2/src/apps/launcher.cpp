
#include <rckid/apps/music_player.h>
#include <rckid/apps/steps.h>

#include <rckid/apps/launcher.h>

namespace rckid {
   
    unique_ptr<ui::Menu> mainMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem{"Steps Counter", assets::icons_64::footprint, []() {
                Steps{}.run();
            }}
            << ui::MenuItem{"Music", assets::icons_64::footprint, []() {
                MusicPlayer{}.run();
            }}
            << ui::MenuItem::Generator("More Apps", assets::icons_64::footprint, mainMenuGenerator)
            << ui::MenuItem{"Steps Two", assets::icons_64::footprint, []() {
                Steps{}.run();
            }};
        return result;
    }
    

} // namespace rckid