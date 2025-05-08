#pragma once

#include "../app.h"
#include "../ui/file_browser.h"
#include "../ui/header.h"
#include "../audio/mp3.h"

namespace rckid {

    /** Simple Music Player. 
     
        This will be simple file browser and when a file is selected, it will start audio decoding and playback.
     */
    class AudioPlayer : public ui::App<void> {
    public:
        AudioPlayer() : ui::App<void>{} {
            // mount the SD card
            filesystem::mount();
            // TODO whatif not mounted? 
            c_ = new FileBrowser{this};
            g_.add(c_);
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            hdr_ = new ui::Header{};
            g_.add(hdr_);
        }

        void update() override {
            ui::App<void>::update();
            c_->processEvents();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                String path = c_->currentPath();
                LOG(LL_DEBUG, "AudioPlayer: starting playback of " << path);
                String ext = filesystem::ext(path);
                filesystem::FileRead f = filesystem::fileRead(path);
                if (ext == ".mp3") {
                    ArenaGuard g{};
                    MP3Stream mp3{f, Arena::allocator()};
                    Player::run(path, mp3);
                } else {
                    LOG(LL_ERROR, "AudioPlayer: unsupported file type " << ext);
                }
            }
        }


    protected:
        class FileBrowser : public ui::FileBrowser {
        public:
            FileBrowser(AudioPlayer * player) : ui::FileBrowser{"/audio"}, player_{player} {
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
            AudioPlayer * player_;
        }; // AudioPlayer::FileBrowser

        class Player : public ui::App<bool> {
        public:
            Player(String path, AudioStream & s) : 
                ui::App<bool>{Rect::XYWH(0, 160, 320, 80)}, 
                as_{s} {
                // TODO
                audioPlay(as_);
            }

            static bool run(String path, AudioStream & s) {
                Player * p = new (Arena::alloc<Player>()) Player{path, s};
                std::optional<bool> res = p->run();
                if (res.has_value())
                    return res.value();
                return false;
            }

            using ui::App<bool>::run;

        protected:

            void update() override {
                // TODO
            }
        private:
            AudioStream & as_;

        }; // AudioPlayer::Player

        FileBrowser * c_;
        ui::Header * hdr_;


    }; // rckid::MusicPlayer

} // namespace rckid