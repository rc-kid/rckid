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

        using FileActionEvent = std::function<void(String path)>;

        String name() const override { return "FileBrowser"; }

        FileBrowser() {
            using namespace ui;
            carousel_ = addChild(new Launcher::BorrowedCarousel());
        }

        /** Menu generator for given folder. 
         
            TODO add filter, and some options, such as sorting, etc.
            TODO add icon settings for different file types
            TODO add decorator support as well
         */
        static unique_ptr<ui::Menu> folderMenuGenerator(FileActionEvent fileAction, String folder, fs::Drive drive) {
            auto result = std::make_unique<ui::Menu>();
            fs::readFolder(folder, drive, [fileAction, folder, drive, & result](fs::FolderEntry const & entry) {
                if (entry.isFolder) {
                    (*result)
                      << ui::MenuItem::Generator(entry.name, assets::icons_64::folder, [fileAction, path = fs::join(folder, entry.name), drive]() {
                            return folderMenuGenerator(fileAction, path, drive);
                        });
                } else {
                    result->emplace_back(entry.name, assets::icons_64::poo, [fileAction, path = fs::join(folder, entry.name)]() {
                        fileAction(path);
                    });
                } 
            });
            return result;
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