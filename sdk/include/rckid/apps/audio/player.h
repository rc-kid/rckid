#include <rckid/timer.h>
#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/utils/file_browser.h>

#include <rckid/audio/playback.h>
#include <rckid/audio/mp3.h>

#include <assets/icons_64.h>
#include <assets/icons_24.h>
#include <assets/OpenDyslexic32.h>
#include <assets/Iosevka24.h>


namespace rckid::audio {

    /** Audio player. 
     
        Given a playback playlist, the app simply plays the playlist and allows choosing between shuffle and 
     */
    class Player : public ui::App<void> {
    public:

        String name() const override { return "AudioPlayer"; }

        Player(Playlist * playlist, uint32_t index = 0):
            ui::App<void>{Rect::XYWH(0, 140, 320, 100)},
            playlist_{playlist},
            index_{index},
            task_{std::make_unique<PlaybackTask>(playlist_, index)}
        {
            using namespace ui;
            playIcon_ = addChild(new Image{})
                << SetRect(Rect::XYWH(0, 0, 100, 100))
                << SetBitmap(assets::icons_64::play_button);
            pauseIcon_ = addChild(new Image{})
                << SetRect(Rect::XYWH(0, 0, 100, 100))
                << SetBitmap(assets::icons_64::pause)
                << SetVisibility(false);
            shuffleIcon_ = addChild(new Image{})
                << SetRect(Rect::XYWH(60, 60, 24, 24))
                << SetBitmap(assets::icons_24::shuffle)
                << SetVisibility(false);
            repeatIcon_ = addChild(new Image{})
                << SetRect(Rect::XYWH(60, 60, 24, 24))
                << SetBitmap(assets::icons_24::exchange)
                << SetVisibility(false);
            playbackTitle_ = addChild(new Label{})
                << SetRect(Rect::XYWH(100, 5, 220, 32))
                << SetFont(assets::OpenDyslexic32);
            playbackDuration_ = addChild(new Label{})
                << SetRect(Rect::XYWH(100, 40, 220, 24))
                << SetFont(assets::Iosevka24);
            root_.useBackgroundImage(false);
            root_.setBg(ui::Style::defaultBg());

            task_->onTrackChanged = [this](uint32_t index) {
                playbackTitle_->setText(playlist_->titleAt(index));
            };

        }

    private:

        void onLoopStart() override {
            ui::App<void>::onLoopStart();
            ASSERT(task_ != nullptr);
            task_->play();
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                if (audio::isPaused()) {
                    audio::resume();
                    pauseIcon_->setVisibility(false);
                    playIcon_->setVisibility(true);
                    task_->resume();
                } else {
                    audio::pause();
                    task_->pause();
                    pauseIcon_->setVisibility(true);
                    playIcon_->setVisibility(false);
                }
            }
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit();
            }
            if (btnPressed(Btn::Start)) {
                if (task_->repeat()) {
                    task_->setRepeat(false);
                    task_->setShuffle(true);
                } else if (task_->shuffle()) {
                    task_->setShuffle(false);
                } else {
                    task_->setRepeat(true);
                }
                shuffleIcon_->setVisibility(task_->shuffle());
                repeatIcon_->setVisibility(task_->repeat());
            }
            if (btnPressed(Btn::Left))
                task_->prev();
            if (btnPressed(Btn::Right))
                task_->next();
        }

        void render() override {
            TinyTime t = task_->elapsed();
            playbackDuration_->setText(STR(nonZero(t.hour(), ":") << alignRight(t.minute(), 2, '0') << ":" << alignRight(t.second(), 2, '0')));
            ui::App<void>::render();
        }

        Playlist * playlist_ = nullptr;
        uint32_t index_ = 0;
        // TODO can task be just normal value and not a pointer? 
        unique_ptr<PlaybackTask> task_;

        ui::Image * playIcon_;
        ui::Image * pauseIcon_;
        ui::Image * shuffleIcon_;
        ui::Image * repeatIcon_;
        ui::Label * playbackTitle_;
        ui::Label * playbackDuration_;

    };

} // namespace rckid::audio