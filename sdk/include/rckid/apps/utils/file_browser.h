#pragma once

#include <rckid/filesystem.h>
#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>

#include <assets/icons_64.h>

namespace rckid {

    /** Simple file browser application. 
     
        Basic app that can perform basic file browsing operations on both the SD card and the cartridge flash memory. 
     */
    class FileBrowser : public ui::Widget {
    public:

        using FileFilter = std::function<bool(String const & path)>;

        using FileActionEvent = std::function<void(String path)>;

        static unique_ptr<LauncherMenu> rootMenuGenerator() {
            return folderMenuGenerator(nullptr, "", fs::Drive::SD);            
        }

        /** Menu generator for given folder. 
         
            TODO and some options, such as sorting, etc.
            TODO add icon settings for different file types
            TODO add decorator support as well
         */
        static unique_ptr<LauncherMenu> folderMenuGenerator(FileActionEvent fileAction, String folder, fs::Drive drive, FileFilter filter = nullptr) {
            auto result = std::make_unique<LauncherMenu>();
            fs::readFolder(folder, drive, [fileAction, folder, drive, filter, & result](fs::FolderEntry const & entry) {
                if (entry.isFolder) {
                    (*result)
                      << ui::MenuItem::Generator(entry.name, assets::icons_64::folder, [fileAction, filter, path = fs::join(folder, entry.name), drive]() {
                            return folderMenuGenerator(fileAction, path, drive, filter);
                        });
                } else if (filter == nullptr || filter(fs::join(folder, entry.name))) {
                    result->emplace_back(entry.name, fileIcon(entry.name), [fileAction, path = fs::join(folder, entry.name)]() {
                        fileAction(path);
                    });
                } 
            });
            return result;
        }

        static bool audioFileFilter(String const & path) {
            if (path.endsWith(".mp3"))
                return true;
            return false;
        }

        static ImageSource fileIcon(String const & path) {
            String ext = fs::ext(path);
            if (ext == "mp3")
                return assets::icons_64::music_1;
            if (ext == "png")
                return assets::icons_64::picture;
            return assets::icons_64::file;
        }

    protected:

    }; // rckid::FileBrowser


} // namespace rckid