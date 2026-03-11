#pragma once

#include <rckid/filesystem.h>
#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>

namespace rckid {

    /** Simple file browser application. 
     
        Basic app that can perform basic file browsing operations on both the SD card and the cartridge flash memory. 
     */
    class FileBrowser : public ui::App<void> {
    public:

        using FileFilter = std::function<bool(String const & path)>;

        using FileActionEvent = std::function<void(String path)>;

        String name() const override { return "FileBrowser"; }

        FileBrowser() {
            using namespace ui;
            carousel_ = addChild(new Launcher::BorrowedCarousel());
        }

        /** Menu generator for given folder. 
         
            TODO and some options, such as sorting, etc.
            TODO add icon settings for different file types
            TODO add decorator support as well
         */
        static unique_ptr<ui::Menu> folderMenuGenerator(FileActionEvent fileAction, String folder, fs::Drive drive, FileFilter filter = nullptr) {
            auto result = std::make_unique<ui::Menu>();
            fs::readFolder(folder, drive, [fileAction, folder, drive, filter, & result](fs::FolderEntry const & entry) {
                if (entry.isFolder) {
                    (*result)
                      << ui::MenuItem::Generator(entry.name, assets::icons_64::folder, [fileAction, filter, path = fs::join(folder, entry.name), drive]() {
                            return folderMenuGenerator(fileAction, path, drive, filter);
                        });
                } else if (filter == nullptr || filter(fs::join(folder, entry.name))) {
                    result->emplace_back(entry.name, assets::icons_64::poo, [fileAction, path = fs::join(folder, entry.name)]() {
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

    protected:

        void onLoopStart() override {
            using namespace ui;
            with(carousel_)
                << ResetMenu([]() { return folderMenuGenerator(nullptr, "", fs::Drive::SD); });
        }

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(carousel_);
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                ASSERT(carousel_->atRoot());
                // TODO terminate music, etc
                exit();
            }
        }

    private:
        Launcher::BorrowedCarousel * carousel_;

    }; // rckid::FileBrowser


} // namespace rckid