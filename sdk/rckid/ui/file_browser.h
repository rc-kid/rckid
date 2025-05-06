#pragma once

#include "../assets/icons_default_64.h"
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
        }

        /** Processes the left and right menu transitions. 
         */
        void processEvents() override {
            // if there is no menu, don't do anything
            if (entries_.size() == 0)
                return;
            // if we have ongoing animation, don't do anything
            if (! idle())
                return;
            if (btnDown(Btn::Left)) {
                i_ = (i_ + entries_.size() - 1) % entries_.size();
                setEntry(i_, Transition::Left);
            }
            if (btnDown(Btn::Right)) {
                i_ = (i_ + 1) % entries_.size();
                setEntry(i_, Transition::Right);
            }
            if (btnDown(Btn::Down) || btnDown(Btn::B)) {
                if (! dirStack_.empty()) {
                    loadDir(filesystem::parent(path_), Transition::Down);
                    // TODO mark event as handled
                }
            }
            if (btnDown(Btn::Up) || btnDown(Btn::A)) {
                if (entries_[i_].isFolder()) {
                    loadDir(filesystem::join(path_, entries_[i_].name()), Transition::Up);
                    // TODO mark event as handled
                } 
            }
        }

    protected:

        void loadDir(String path, Transition transition = Transition::Up) {
            LOG(LL_INFO, "Loading dir " << path);
            if (transition == Transition::Up) {
                if (path != "/")
                    dirStack_.push_back(i_);
                i_ = 0;
            } else {
                i_ = dirStack_.back();
                dirStack_.pop_back();
            }
            path_ = std::move(path);
            entries_.clear();
            filesystem::Folder folder = filesystem::folderRead(path_.c_str(), drive_);
            for (auto entry : folder)
                entries_.push_back(entry);
            setEntry(i_, transition);
        }

        Bitmap<ColorRGB> getIconFor(filesystem::Entry const & entry) {
            ArenaGuard g{};
            if (entry.isFolder()) {
                return Bitmap<ColorRGB>{PNG::fromBuffer(assets::icons_default_64::folder)};
            } else {
                String ext = filesystem::ext(entry.name());
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                    return Bitmap<ColorRGB>{PNG::fromBuffer(assets::icons_default_64::paint_palette)};
                else if (ext == ".mp3") 
                    return Bitmap<ColorRGB>{PNG::fromBuffer(assets::icons_default_64::music_note)};
                else
                    return Bitmap<ColorRGB>{PNG::fromBuffer(assets::icons_default_64::notes)};
            }
        }

        void setEntry(uint32_t i, Transition transition) {
            set(filesystem::stem(entries_[i].name()), getIconFor(entries_[i]), transition);
        }

    private:
        filesystem::Drive drive_ = filesystem::Drive::SD;
        String path_;
        uint32_t i_ = 0;
        std::vector<filesystem::Entry> entries_;
        std::vector<uint32_t> dirStack_;
    }; 

}