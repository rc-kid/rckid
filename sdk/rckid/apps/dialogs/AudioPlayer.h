#pragma once

#include "../../ui/form.h"
#include "../../ui/header.h"
#include "../../ui/label.h"
#include "../../audio/mp3.h"
#include "../../assets/icons_64.h"
#include "../../assets/icons_24.h"
#include "../../assets/fonts/OpenDyslexic32.h"


namespace rckid {

    enum class AudioPlayerResult {
        Stop,
        Done,
        PlayPrev,
        PlayNext
    }; // AudioPlayerResult

    enum class AudioPlayerMode {
        Normal,
        Repeat,
        Shuffle
    }; // AudioPlayerMode

    class AudioPlayer : public ui::Form<void> {
    public:

        class Playlist {
        public:

            virtual ~Playlist() = default;

            /** Creates audio stream from the current playlist item. 
             */
            virtual AudioStream * current() = 0;

            virtual bool next(bool shuffle = false) = 0;

            virtual bool prev(bool shuffle = false) = 0;

            virtual bool supportsShuffle() const { return false; }

        }; // AudioPlayer::Playlist

        /** Single file playlist.
         */
        class SingleFile : public Playlist {
        public:

            SingleFile(String path): path_{path} { }

            AudioStream * current() override { return AudioStream::fromFile(path_); }

            bool next(bool /*shuffle*/ = false) override { return false; }
            bool prev(bool /*shuffle*/ = false) override { return false; }

        private:
            String path_;
        }; 

        String name() const override { return "AudioPlayer"; }

        /** Shorthand for creating audio player for a single file.
         */
        AudioPlayer(String filename):
            AudioPlayer(std::make_unique<SingleFile>(filename)) { }

        /** Audio player constructor with proper playlist.
         */
        AudioPlayer(std::unique_ptr<Playlist> playlist):
            ui::Form<void>{Rect::XYWH(0, 144, 320, 96), /*raw*/ true}, 
            playlist_{std::move(playlist)},
            task_{new PlayerTask{this}},
            title_{88,18,""},
            elapsed_{88,62,""},
            icon_{8,16,Icon{assets::icons_64::play_button}},
            repeatIcon_{50, 58, Icon{assets::icons_24::exchange}},
            shuffleIcon_{50, 58, Icon{assets::icons_24::shuffle}}
        {
            title_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            g_.addChild(title_);
            g_.addChild(elapsed_);
            g_.addChild(icon_);
            g_.addChild(repeatIcon_);
            g_.addChild(shuffleIcon_);
            repeatIcon_.setVisible(false);
            shuffleIcon_.setVisible(false);
            playStream(playlist_->current());
        }

        ~AudioPlayer() override {
            delete task_;
            delete as_;
        }

    protected:

        class PlayerTask : public Task {
        public:

            PlayerTask(AudioPlayer * player) : player_{player} { }

            ~PlayerTask() override {
                audioStop();
            }

            void run() override {
                player_->updateAudio();
            }

        private:

            AudioPlayer * player_;

        }; // PlayerTask

        void updateAudio() {
            if (as_ == nullptr)
                return;
            if (!audioPaused())
                keepAlive();
            as_->update();
            if (!audioPlayback()) {
                if (mode_ == AudioPlayerMode::Repeat) {
                    delete as_;
                    playStream(playlist_->current());
                } else {
                    playNext();
                }
            }
        }

        void playNext() {
            delete as_;
            if (playlist_->next(mode_ == AudioPlayerMode::Shuffle))
                playStream(playlist_->current());
            else
                as_ = nullptr;
        }

        void playPrev() {
            delete as_;
            if (playlist_->prev(mode_ == AudioPlayerMode::Shuffle))
                playStream(playlist_->current());
            else
                as_ = nullptr;
        }

        void playStream(AudioStream * s) {
            as_ = s;
            if (as_ != nullptr) {
                audioPlay(*as_);
                lastUs_ = uptimeUs();
                elapsedUs_ = 0;
                title_.setText(as_->name());
            }
            // TODO reset clock, update title label
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
            // only redraw if there is change in the elapsed time
            if (elapsed_.setText(sw.str()))
                redraw_ = true;
        }

        void update() override {
            ui::Form<void>::update();
            // deal with controls
            if (! audioPaused()) {
                uint32_t t = uptimeUs();
                elapsedUs_ += t - lastUs_;
                lastUs_ = t;
                setElapsedTime();
            }
            // when back or down is pressed, return from the player mode
            if (btnPressed(Btn::B) || btnPressed(Btn::Down) || as_ == nullptr) {
                exit();
            }
            // btn up, or button A is audio pause
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                if (audioPaused()) {
                    icon_ = Icon{assets::icons_64::play_button};
                    audioResume();
                    lastUs_ = uptimeUs();
                } else {
                    icon_ = Icon{assets::icons_64::pause};
                    audioPause();
                }
                redraw_ = true;
            }
            if (btnPressed(Btn::Left)) {
                btnClear(Btn::Left);
                playPrev();
            }
            if (btnPressed(Btn::Right)) {
                btnClear(Btn::Right);
                playNext();
            }
            if (btnPressed(Btn::Start)) {
                switch (mode_) {
                    case AudioPlayerMode::Normal:
                        mode_ = AudioPlayerMode::Repeat;
                        repeatIcon_.setVisible(true);
                        shuffleIcon_.setVisible(false);
                        break;
                    case AudioPlayerMode::Repeat:
                        if (playlist_->supportsShuffle()) {
                            mode_ = AudioPlayerMode::Shuffle;
                            repeatIcon_.setVisible(false);
                            shuffleIcon_.setVisible(true);
                            break;
                        }
                        [[fallthrough]]
                    case AudioPlayerMode::Shuffle:
                        mode_ = AudioPlayerMode::Normal;
                        repeatIcon_.setVisible(false);
                        shuffleIcon_.setVisible(false);
                        break;
                    default:
                        UNREACHABLE;
                }
                redraw_ = true;
            }
        }

        /** Only redraw if there is change in the visual elements. This saves precious CPU time on the device for the audio decoding. As it effectively limits the FPS to 1 frane per second. 
         */
        void draw() override {
            if (redraw_ || true) {
                redraw_ = false;
                //elapsed_.setText(STR(as_->underflowCount()));
                ui::Form<void>::draw(); 
            } else {
                displayWaitVSync();
            }
        }

    private:

        std::unique_ptr<Playlist> playlist_;
        PlayerTask * task_;
        AudioStream * as_;
        AudioPlayerMode mode_ = AudioPlayerMode::Normal;
        bool redraw_ = false;

        ui::Label title_;
        ui::Label elapsed_;
        ui::Image icon_;
        ui::Image repeatIcon_;
        ui::Image shuffleIcon_;
        uint64_t elapsedUs_ = 0;
        uint32_t lastUs_ = 0;

    }; // AudioPlayer



} // namespace rckid