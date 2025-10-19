#pragma once

#include "../app.h"
#include "../ui/file_browser.h"
#include "../ui/header.h"
#include "../audio/mp3.h"
#include "../assets/icons_64.h"
#include "../assets/fonts/OpenDyslexic32.h"
#include "dialogs/AudioPlayer.h"

namespace rckid {

    /** Simple Music Player. 
     
        This will be simple file browser and when a file is selected, it will start audio decoding and playback.
     */
    class MusicPlayer : public ui::Form<void> {
    public:

        String name() const override { return "AudioPlayer"; }

        MusicPlayer() : ui::Form<void>{} {
            c_ = g_.addChild(new FileBrowser{this});
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
        }

    protected:

        void update() override {
            ui::Form<void>::update();
            c_->processEvents();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                btnClear(Btn::A);
                btnClear(Btn::Up);
                std::optional<AudioPlayerResult> res;
                // to deal with shuffle and repeat modes, the playback is in a loop
                while (true) {
                    String path = c_->currentPath();
                    if (fs::isFile(path)) {
                        LOG(LL_DEBUG, "AudioPlayer: starting playback of " << path);
                        String ext = fs::ext(path);
                        fs::FileRead f = fs::fileRead(path);
                        if (!f.good()) {
                            // this should really not happen, ever
                            InfoDialog::error("File not found", STR("Cannot open file " << path));
                            break;
                        }
                        if (ext == ".mp3") {
                            MP3Stream mp3{f};
                            res = App::run<AudioPlayer>(path, mp3, mode_);
                        } else {
                            InfoDialog::error("Unsupported format", STR("File format " << ext << " is not supported"));
                            break;
                        }
                    }
                    // now deal with rhe result
                    if (!res.has_value()) {
                        LOG(LL_ERROR, "Unexpected error during playback");
                        break;
                    }
                    // user explicitly stopped the playback, break the loop
                    if (res.value() == AudioPlayerResult::Stop) {
                        break;
                    }
                    if (res.value() == AudioPlayerResult::Done) {
                        if (mode_ == AudioPlayerMode::Repeat) {
                            // continue to play the same file
                            continue;
                        } else if (mode_ == AudioPlayerMode::Shuffle) {
                            res = AudioPlayerResult::PlayNext;
                        } 
                    }
                    if (res.value() == AudioPlayerResult::PlayNext) {
                        uint32_t next = mode_ == AudioPlayerMode::Shuffle ? 
                            (rand() % c_->size()) : 
                            c_->getIndexRight();
                        c_->setItem(next, Direction::None);
                    }
                    if (res.value() == AudioPlayerResult::PlayPrev) {
                        uint32_t next = mode_ == AudioPlayerMode::Shuffle ? 
                            (rand() % c_->size()) : 
                            c_->getIndexLeft();
                        c_->setItem(next, Direction::None);
                    }
                }
            }
        }

        class FileBrowser : public ui::FileBrowser {
        public:
            FileBrowser(MusicPlayer * player) : ui::FileBrowser{"/files/music"}, player_{player} {
            }

        protected:

            void onFileChanged() override {
            }

            void onFolderChanged() override {

            }

            bool onFileFilter(String const & name) override {
                if (name.endsWith(".mp3"))
                    return true;
                // TODO orher formats
                return false;
            }

        private:
            MusicPlayer * player_;
        }; // MusicPlayer::FileBrowser

        FileBrowser * c_;
        AudioPlayerMode mode_ = AudioPlayerMode::Normal;

    }; // rckid::MusicPlayer

} // namespace rckid