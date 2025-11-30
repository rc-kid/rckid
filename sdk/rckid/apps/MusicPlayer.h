#pragma once

#include "../app.h"
#include "../task.h"
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

        MusicPlayer() : 
            ui::Form<void>{}
        {
            c_ = g_.addChild(new FileBrowser{this});
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
        }

    protected:

        /** A very simple playlist that is connected to the music player file display and allows playing multiple files from the playlist.
         
            NOTE that the playlist requires the music player to exist, i.e. it is not possible to run the audio player or its background task without the music player being also on the app stack. This enables listening to music while in home menu, or when the screen is locked, but not while running other apps than the music player.
         */
        class FolderPlaylist : public AudioPlayer::Playlist {
        public:
            FolderPlaylist(MusicPlayer & player): player_{player} {}

            AudioStream * current() override { return AudioStream::fromFile(player_.c_->currentPath()); }

            bool next(bool shuffle = false) override {
                while (true) {
                    uint32_t next = shuffle ? 
                        (rand() % player_.c_->size()) : 
                        player_.c_->getIndexRight();
                    player_.c_->setItem(next, Direction::None);
                    if (fs::isFile(player_.c_->currentPath()))
                        return true;
                }
            }

            bool prev(bool shuffle = false) override {
                while (true) {
                    uint32_t next = shuffle ? 
                        (rand() % player_.c_->size()) : 
                        player_.c_->getIndexLeft();
                    player_.c_->setItem(next, Direction::None);
                    if (fs::isFile(player_.c_->currentPath()))
                        return true;
                }
            }

            bool supportsShuffle() const override { return true; }

        private:

            MusicPlayer & player_;

        }; // MusicPlayer::FolderPlaylist


        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                btnClear(Btn::A);
                btnClear(Btn::Up);
                App::run<AudioPlayer>(std::make_unique<FolderPlaylist>(*this));
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

        void focus() override {
            ui::Form<void>::focus();
            c_->focus();
        }

        FileBrowser * c_;
        AudioPlayerMode mode_ = AudioPlayerMode::Normal;

    }; // rckid::MusicPlayer

} // namespace rckid