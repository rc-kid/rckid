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
                String path = c_->currentPath();
                LOG(LL_DEBUG, "AudioPlayer: starting playback of " << path);
                String ext = fs::ext(path);
                fs::FileRead f = fs::fileRead(path);
                if (!f.good()) {
                    LOG(LL_ERROR, "AudioPlayer: file not found " << path);
                    return;
                }
                if (ext == ".mp3") {
                    MP3Stream mp3{f};
                    auto res = App::run<AudioPlayer>(path, mp3);
                    if (res.has_value()) {
                        // TODO deal with the result
                    }
                } else {
                    LOG(LL_ERROR, "AudioPlayer: unsupported file type " << ext);
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

    }; // rckid::MusicPlayer

} // namespace rckid