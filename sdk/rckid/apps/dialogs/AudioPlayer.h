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

    class AudioPlayer : public ui::Form<AudioPlayerResult> {
    public:

        /** Use umbrella name for all audio player stuff. 
         */
        String name() const override { return "AudioPlayer"; }

        AudioPlayer(String path, AudioStream & s, AudioPlayerMode & mode) : 
            AudioPlayer{path, s} {
            mode_ = &mode;
            repeatIcon_.setVisible(mode == AudioPlayerMode::Repeat);
            shuffleIcon_.setVisible(mode == AudioPlayerMode::Shuffle);
        }

        AudioPlayer(String path, AudioStream & s):
            ui::Form<AudioPlayerResult>{Rect::XYWH(0, 144, 320, 96), /*raw*/ true}, 
            as_{s},
            title_{88,18,fs::stem(path)},
            elapsed_{88,62,String{""}},
            icon_{8,16,Icon{assets::icons_64::play_button}},
            repeatIcon_{50, 58, Icon{assets::icons_24::exchange}},
            shuffleIcon_{50, 58, Icon{assets::icons_24::shuffle}} {
            audioPlay(as_);
            lastUs_ = uptimeUs();
            elapsedUs_ = 0;
            title_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            g_.addChild(title_);
            g_.addChild(elapsed_);
            g_.addChild(icon_);
            g_.addChild(repeatIcon_);
            g_.addChild(shuffleIcon_);
            repeatIcon_.setVisible(false);
            shuffleIcon_.setVisible(false);
        }

        ~AudioPlayer() override {
            audioStop();
        }

    protected:
        void update() override {
            ui::Form<AudioPlayerResult>::update();
            as_.update();
            // deal with controls
            if (! audioPaused()) {
                uint32_t t = uptimeUs();
                elapsedUs_ += t - lastUs_;
                lastUs_ = t;
                setElapsedTime();
                // when not paused, keep the device alive
                keepAlive();
            }
            // when back or down is pressed, return from the player mode
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                btnClear(Btn::B);
                btnClear(Btn::Down);
                exit(AudioPlayerResult::Stop);
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
                exit(AudioPlayerResult::PlayPrev);
            }
            if (btnPressed(Btn::Right)) {
                btnClear(Btn::Right);
                exit(AudioPlayerResult::PlayNext);
            }
            if (mode_ != nullptr && btnPressed(Btn::Start)) {
                switch (*mode_) {
                    case AudioPlayerMode::Normal:
                        *mode_ = AudioPlayerMode::Repeat;
                        repeatIcon_.setVisible(true);
                        shuffleIcon_.setVisible(false);
                        break;
                    case AudioPlayerMode::Repeat:
                        *mode_ = AudioPlayerMode::Shuffle;
                        repeatIcon_.setVisible(false);
                        shuffleIcon_.setVisible(true);
                        break;
                    case AudioPlayerMode::Shuffle:
                        *mode_ = AudioPlayerMode::Normal;
                        repeatIcon_.setVisible(false);
                        shuffleIcon_.setVisible(false);
                        break;
                    default:
                        UNREACHABLE;
                }
                redraw_ = true;
            }
            // TODO btn left & right is immediate return
            if (!audioPlayback())
                exit(AudioPlayerResult::Done);
        }

        /** Only redraw if there is change in the visual elements. This saves precious CPU time on the device for the audio decoding. As it effectively limits the FPS to 1 frane per second. 
         */
        void draw() override {
            if (redraw_) {
                redraw_ = false;
                ui::Form<AudioPlayerResult>::draw(); 
            } else {
                displayWaitVSync();
            }
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
    private:
        AudioStream & as_;
        ui::Label title_;
        ui::Label elapsed_;
        ui::Image icon_;
        ui::Image repeatIcon_;
        ui::Image shuffleIcon_;
        uint64_t elapsedUs_ = 0;
        uint32_t lastUs_ = 0;
        // when true, the player window will redraw itself
        bool redraw_ = false;

        AudioPlayerMode * mode_ = nullptr;

    }; // AudioPlayer::Player


} // namespace rckid