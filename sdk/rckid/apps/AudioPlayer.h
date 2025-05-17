#pragma once

#include "../app.h"
#include "../ui/file_browser.h"
#include "../ui/header.h"
#include "../audio/mp3.h"
#include "../assets/icons_default_64.h"
#include "../assets/fonts/OpenDyslexic32.h"

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
                if (!f.good()) {
                    LOG(LL_ERROR, "AudioPlayer: file not found " << path);
                    return;
                }
                if (ext == ".mp3") {
                    NewArenaGuard g{};
                    MP3Stream mp3{f};
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
                lastUs_ = uptimeUs();
                elapsedUs_ = 0;
                title_ = new ui::Label{80, 10, filesystem::stem(path)};
                title_->setFont(Font::fromROM<assets::OpenDyslexic32>());
                elapsed_ = new ui::Label{80, 54, String{""}};
                icon_ = new ui::Image{Bitmap<ColorRGB>{PNG::fromBuffer(assets::icons_default_64::play)}};
                icon_->setPos(8,8);
                g_.add(title_);
                g_.add(icon_);
                g_.add(elapsed_);
            }

            ~Player() override {
                audioStop();
            }

            static bool run(String path, AudioStream & s) {
                Player * p = new (Arena::alloc<Player>()) Player{path, s};
                std::optional<bool> res = p->run();
                delete p;
                if (res.has_value())
                    return res.value();
                return false;
            }

            using ui::App<bool>::run;

        protected:

            void update() override {
                ui::App<bool>::update();
                if (! audioPaused()) {
                    uint32_t t = uptimeUs();
                    elapsedUs_ += t - lastUs_;
                    lastUs_ = t;
                    setElapsedTime();
                }
                // when back or down is pressed, return from the player mode
                if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                    btnClear(Btn::B);
                    btnClear(Btn::Down);
                    exit(false);
                }
                // btn up, or button A is audio pause
                if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                    if (audioPaused()) {
                        icon_->setBitmap(Bitmap<ColorRGB>{PNG::fromBuffer(assets::icons_default_64::play)});
                        audioResume();
                        lastUs_ = uptimeUs();
                    } else {
                        icon_->setBitmap(Bitmap<ColorRGB>{PNG::fromBuffer(assets::icons_default_64::pause)});
                        audioPause();
                    }
                }
                // btn left & right is immediate return
                if (!audioPlayback())
                    exit(true);
            }

            void setElapsedTime() {
                uint32_t seconds = elapsedUs_ / 1000000;
                uint32_t minutes = seconds / 60;
                uint32_t hours = minutes / 60;
                seconds = seconds % 60;
                minutes = minutes % 60;
                StringWriter sw;
                if (hours > 0)
                    sw << hours << ":";
                sw << fillLeft(minutes, 2, '0') << ":" << fillLeft(seconds, 2, '0');
                elapsed_->setText(sw.str());
            }
        private:
            AudioStream & as_;
            ui::Label * title_;
            ui::Label * elapsed_;
            ui::Image * icon_;
            uint64_t elapsedUs_ = 0;
            uint32_t lastUs_ = 0;

        }; // AudioPlayer::Player

        FileBrowser * c_;
        ui::Header * hdr_;

    }; // rckid::MusicPlayer

} // namespace rckid