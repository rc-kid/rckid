#pragma once

#include "../../ui/form.h"
#include "../../ui/header.h"
#include "../../ui/label.h"
#include "../../audio/mp3.h"
#include "../../assets/icons_64.h"
#include "../../assets/fonts/OpenDyslexic32.h"


namespace rckid {

    enum class AudioPlayerResult {
        Done,
        PlayPrev,
        PlayNext
    }; // AudioPlayerResult

    class AudioPlayer : public ui::Form<AudioPlayerResult> {
    public:

        /** Use umbrella name for all audio player stuff. 
         */
        String name() const override { return "AudioPlayer"; }

        AudioPlayer(String path, AudioStream & s) : 
            ui::Form<AudioPlayerResult>{Rect::XYWH(0, 160, 320, 80), /*raw*/ true}, 
            as_{s},
            title_{80,10,fs::stem(path)},
            elapsed_{80,54,String{""}},
            icon_{8,8,Icon{assets::icons_64::play_button}} {
            // TODO
            as_.update();
            audioPlay(as_);
            lastUs_ = uptimeUs();
            elapsedUs_ = 0;
            title_.setFont(Font::fromROM<assets::OpenDyslexic32>());
            g_.addChild(title_);
            g_.addChild(elapsed_);
            g_.addChild(icon_);
        }

        ~AudioPlayer() override {
            audioStop();
        }

    protected:

        void update() override {
            // TODO I really have no idea what is going on, no matter how high number I put here, the memory just keeps being a bit above that, so when 200k it will keep printing a bit over 200k, when 400k it will print a bit above 400k
            // WHY WHY WHY? 
            if (RAMHeap::usedBytes() > 400000)
                LOG(LL_INFO, "Used bytes: " << RAMHeap::usedBytes());
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
                exit(AudioPlayerResult::Done);
            }
            /*
            // btn up, or button A is audio pause
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                LOG(LL_INFO, "btn down");
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
                */
            // TODO btn left & right is immediate return
            if (!audioPlayback())
                exit(AudioPlayerResult::PlayNext);
        }

        /** Only redraw if there is change in the visual elements. This saves precious CPU time on the device for the audio decoding. As it effectively limits the FPS to 1 frane per second. 
         */
        void draw() override {
            if (redraw_) {
                redraw_ = false;
                ui::Form<AudioPlayerResult>::draw(); 
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
        uint64_t elapsedUs_ = 0;
        uint32_t lastUs_ = 0;
        // when true, the player window will redraw itself
        bool redraw_ = false;

    }; // AudioPlayer::Player


} // namespace rckid