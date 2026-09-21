#pragma once

#include <rckid/timer.h>
#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/utils/file_browser.h>
#include <rckid/apps/audio/player.h>

#include <rckid/audio/playback.h>
#include <rckid/audio/mp3.h>

#include <assets/icons_64.h>
#include <assets/icons_24.h>
#include <assets/OpenDyslexic32.h>
#include <assets/Iosevka24.h>

namespace rckid {

    /** Simple music player
     
        Allows browsing music files organized in folders on the SD card and playing them via folder playlist and the default audio player dialog. 

        TODO At the moment does not have any useful overlay, but eventually the overlay would display the selected track information (perhaps the album art?)
     */
    class MusicPlayer : public LauncherOverlay {
    public:
        static unique_ptr<LauncherMenu> generateLauncherMenu() {
            auto menu = FileBrowser::folderMenuGenerator([](String path){
                // take the folder of the file, create playlist from it, found the current file and start playback from it 
                String folder = fs::parent(path);
                String filename = fs::filename(path);
                auto playlist = std::make_unique<audio::FolderPlaylist>(folder);
                App::run<audio::Player>(playlist.get(), playlist->indexOf(filename));
            }, "/files/music", fs::Drive::SD, FileBrowser::audioFileFilter);
            return menu;
        }

    }; // rckid::MusicPlayer

} // namespace rckid