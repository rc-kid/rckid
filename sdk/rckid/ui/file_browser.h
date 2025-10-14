#pragma once

#include "../assets/icons_64.h"
#include "../filesystem.h"
#include "carousel.h"

namespace rckid::ui {

    /** File browser. 
     
        Ideally customizable with some extra events that allow specifying the icon, etc. 


     */
    class FileBrowser : public Carousel {
    public:

        FileBrowser(String dir) {
            loadDir(std::move(dir));
            // if we started not in a root, clear the dir stack so that the file explorer will not allow going under the root it was launched in
            dirStack_.clear();
        }

        String currentPath() const { 
            if (entries_.size() == 0)
                return path_;
            return fs::join(path_, entries_[currentIndex()].name());
        }

        /** Processes the left and right menu transitions. 
         */
        bool processEvents() override {
            // if there is no menu, don't do anything
            if (entries_.size() == 0)
                return true;
            // if we have ongoing animation, don't do anything
            if (! idle())
                return true;

            Carousel::processEvents();

            if (btnDown(Btn::Down) || btnDown(Btn::B)) {
                if (! dirStack_.empty()) {
                    loadDir(fs::parent(path_), Direction::Down);
                    // clear the button state (cancellation is handled by button press)
                    btnClear(Btn::Down);
                    btnClear(Btn::B);
                }
            }
            if (btnDown(Btn::Up) || btnDown(Btn::A)) {
                if (entries_[currentIndex()].isFolder()) {
                    loadDir(fs::join(path_, entries_[currentIndex()].name()), Direction::Up);
                    // clear the button state (selection is handled by button press)
                    btnClear(Btn::Up);
                    btnClear(Btn::A);
                } 
            }
            return true;
        }

        uint32_t size() const override { return entries_.size(); }

    protected:

        void doSetItem(uint32_t index, Direction direction) override {
            set(fs::stem(entries_[index].name()), getIconFor(entries_[index]), direction);
            if (entries_[index].isFolder())
                onFolderChanged();
            else
                onFileChanged();
        }

        virtual void onFileChanged() {}
        virtual void onFolderChanged() {}
        virtual bool onFileFilter([[maybe_unused]] String const & name) { return true; }

    protected:

        void loadDir(String path, Direction transition = Direction::Up) {
            LOG(LL_INFO, "Loading dir " << path);
            uint32_t newIndex = 0;
            if (transition == Direction::Up) {
                if (path != "/")
                    dirStack_.push_back(currentIndex());
            } else {
                newIndex = dirStack_.back();
                dirStack_.pop_back();
            }
            path_ = std::move(path);
            entries_.clear();
            fs::Folder folder = fs::folderRead(path_.c_str(), drive_);
            for (auto & entry : folder) {
                if (entry.isFile() && ! onFileFilter(entry.name()))
                    continue;
                LOG(LL_DEBUG, "FileBrowser: adding entry " << entry.name());
                entries_.push_back(entry);
            }
            if (entries_.empty()) {
                showEmpty(transition);
            } else {
                setItem(newIndex, transition);
            }
        }

        Icon getIconFor(fs::Entry const & entry) {
            if (entry.isFolder()) {
                return Icon{assets::icons_64::folder};
            } else {
                String ext = fs::ext(entry.name());
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                    return Icon{assets::icons_64::paint_palette};
                else if (ext == ".mp3") 
                    return Icon{assets::icons_64::music_note};
                else if (ext == ".gb" || ext == ".gbc")
                    return Icon{assets::icons_64::gameboy};
                else
                    return Icon{assets::icons_64::notes};
            }
        }

        /*

        void setEntry(uint32_t i, Direction transition) {
            set(fs::stem(entries_[i].name()), getIconFor(entries_[i]), transition);
            if (entries_[i].isFolder())
                onFolderChanged();
            else
                onFileChanged();
        }

        */

    private:
        fs::Drive drive_ = fs::Drive::SD;
        String path_;
        std::vector<fs::Entry> entries_;
        std::vector<uint32_t> dirStack_;
    }; 

}