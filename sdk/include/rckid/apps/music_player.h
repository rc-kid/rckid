#pragma once

#include <rckid/timer.h>
#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/file_browser.h>

#include <rckid/audio/playback.h>
#include <rckid/audio/mp3.h>

#include <assets/icons_64.h>
#include <assets/icons_24.h>
#include <assets/OpenDyslexic32.h>
#include <assets/Iosevka24.h>

namespace rckid {

    /** Simple music player. 
     */
    class MusicPlayer : public ui::App<void> {
    public:

        String name() const override { return "Music"; }

        MusicPlayer() {
            using namespace ui;
            carousel_ = addChild(new Launcher::BorrowedCarousel());

            playbackInfo_ = addChild(new Panel{})
                << SetRect(Rect::XYWH(0, 140, 320, 100))
                << SetVisibility(false);
            playIcon_ = playbackInfo_->addChild(new Image{})
                << SetRect(Rect::XYWH(-100, 0, 100, 100))
                << SetBitmap(assets::icons_64::play_button);
            pauseIcon_ = playbackInfo_->addChild(new Image{})
                << SetRect(Rect::XYWH(-100, 0, 100, 100))
                << SetBitmap(assets::icons_64::pause)
                << SetVisibility(false);
            shuffleIcon_ = playbackInfo_->addChild(new Image{})
                << SetRect(Rect::XYWH(-40, 60, 24, 24))
                << SetBitmap(assets::icons_24::shuffle)
                << SetVisibility(false);
            repeatIcon_ = playbackInfo_->addChild(new Image{})
                << SetRect(Rect::XYWH(-40, 60, 24, 24))
                << SetBitmap(assets::icons_24::exchange)
                << SetVisibility(false);
            playbackTitle_ = playbackInfo_->addChild(new Label{})
                << SetRect(Rect::XYWH(320, 5, 220, 32))
                << SetFont(assets::OpenDyslexic32);
            playbackDuration_ = playbackInfo_->addChild(new Label{})
                << SetRect(Rect::XYWH(320, 40, 220, 24))
                << SetFont(assets::Iosevka24);
        }

    protected:

        /** Audio playlist for an entire folder, backed by the music player.
         
            As the music player, the folder playlist fixes the playlist to the SD card. 
         */
        class FolderPlaylist : public audio::Playlist {
        public:

            /** Creates the playlist without starting the playback.
             */
            FolderPlaylist(MusicPlayer * player): 
                player_{player} {
            }

            /** Starts the playback.
             */
            void start() {
                playbackTask_ = std::make_unique<audio::Playback>(this);
            }

            unique_ptr<audio::DecoderStream> next() override {
                if (index_ == NOT_INITIALIZED)
                    index_ = player_->carousel_->index();
                else
                    moveToNext();
                while (! player_->carousel_->menu()->at(index_).isAction())
                    moveToNext();
                // calling the action will set our path
                player_->carousel_->menu()->at(index_).action()();
                player_->playbackTitle_->setText(fs::stem(player_->playlist_->currentPath_));
                // reset the timer
                player_->timer_.start();
                // create and return the stream
                return audio::DecoderStream::fromFile(currentPath_, fs::Drive::SD);
            }

            unique_ptr<audio::DecoderStream> prev() override {
                if (index_ == NOT_INITIALIZED)
                    index_ = player_->carousel_->index();
                else 
                    moveToPrev();
                while (! player_->carousel_->menu()->at(index_).isAction())
                    moveToPrev();
                // calling the action will set our path
                player_->carousel_->menu()->at(index_).action()();
                player_->playbackTitle_->setText(fs::stem(player_->playlist_->currentPath_));
                // reset the timer
                player_->timer_.start();
                // create and return the stream
                return audio::DecoderStream::fromFile(currentPath_, fs::Drive::SD);
            }

        private:

            friend class MusicPlayer;

            void setCurrentPath(String path) {
                currentPath_ = std::move(path);
            }

            void moveToNext() {
                if (player_->shuffle_)
                    index_ = rand() % player_->carousel_->menu()->size();
                else if (! player_->repeat_)
                    index_ = player_->carousel_->nextIndex();
                player_->carousel_->setItem(index_);
            }

            void moveToPrev() {
                if (player_->shuffle_)
                    index_ = rand() % player_->carousel_->menu()->size();
                else if (! player_->repeat_)
                    index_ = player_->carousel_->prevIndex();
                player_->carousel_->setItem(index_);
            }

            static constexpr uint32_t NOT_INITIALIZED = 0xffffffff;

            MusicPlayer * player_;
            std::unique_ptr<audio::Playback> playbackTask_;
            uint32_t index_ = NOT_INITIALIZED;
            String currentPath_;

        }; 

        void onLoopStart() override {
            using namespace ui;
            with(carousel_)
                << ResetMenu([this]() { return FileBrowser::folderMenuGenerator([this](String path){
                    ASSERT(playlist_ != nullptr);
                    playlist_->setCurrentPath(std::move(path));
                    // TODO and display that we are playing that file, etc.
                }, "/files/music", fs::Drive::SD, FileBrowser::audioFileFilter); });
        }

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(carousel_);
        }

        void render() {
            ui::App<void>::render();
            if (playlist_ != nullptr) {
                timer_.tick();
                TinyTime t = timer_.time();
                playbackDuration_->setText(STR(nonZero(t.hour(), ":") << alignRight(t.minute(), 2, '0') << ":" << alignRight(t.second(), 2, '0')));
            }
        }

        void loop() override {
            ui::App<void>::loop();
                if (playlist_ == nullptr) {
                // when file is selected, simply ensure that the playlist is created and start the playback. This will call the action via next
                if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                    btnClear(Btn::A);
                    btnClear(Btn::Up);
                    focusWidget(nullptr);
                    ASSERT(playlist_ == nullptr);
                    // animate the transition to playback
                    playbackInfo_->setVisibility(true);
                    shuffleIcon_->setVisibility(shuffle_);
                    repeatIcon_->setVisibility(repeat_);
                    playlist_ = std::make_unique<FolderPlaylist>(this);
                    playlist_->start();
                    animate()
                        << ui::FlyOut(pauseIcon_, Point{100, 0})
                        << ui::FlyOut(playIcon_, Point{100, 0})
                        << ui::FlyOut(shuffleIcon_, Point{100, 0})->setDelayMs(animationSpeed() / 2)
                        << ui::FlyOut(repeatIcon_, Point{100, 0})->setDelayMs(animationSpeed() / 2)
                        << ui::FlyOut(playbackTitle_, Point{-220, 0})
                        << ui::FlyOut(playbackDuration_, Point{-220, 0})->setDelayMs(animationSpeed() / 2);
                    waitUntilIdle();
                }
                if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                    ASSERT(carousel_->atRoot());
                    ASSERT(playlist_ == nullptr);
                    exit();
                }
            } else {
                if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                    if (audio::isPaused()) {
                        audio::resume();
                        pauseIcon_->setVisibility(false);
                        playIcon_->setVisibility(true);
                        timer_.resume();
                    } else {
                        audio::pause();
                        timer_.pause();
                        pauseIcon_->setVisibility(true);
                        playIcon_->setVisibility(false);
                    }
                }
                if (btnPressed(Btn::Start)) {
                    if (repeat_) {
                        repeat_ = false;
                        shuffle_ = true;
                    } else if (shuffle_) {
                        shuffle_ = false;
                    } else {
                        repeat_ = true;
                    }
                    shuffleIcon_->setVisibility(shuffle_);
                    repeatIcon_->setVisibility(repeat_);
                }
                if (btnPressed(Btn::Left))
                    playlist_->playbackTask_->prev();
                if (btnPressed(Btn::Right))
                    playlist_->playbackTask_->next();
                if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                    btnClear(Btn::B);
                    btnClear(Btn::Down);
                    // delete playlist, which also destroys the playback task and stops the music
                    playlist_ = nullptr;
                    focusWidget(carousel_);
                    // animate the transition back to carousel
                    animate()
                        << ui::FlyOut(pauseIcon_, Point{-100, 0})->setDelayMs(animationSpeed() / 2)
                        << ui::FlyOut(playIcon_, Point{-100, 0})->setDelayMs(animationSpeed() / 2)
                        << ui::FlyOut(shuffleIcon_, Point{-100, 0})
                        << ui::FlyOut(repeatIcon_, Point{-100, 0})
                        << ui::FlyOut(playbackTitle_, Point{220, 0})->setDelayMs(animationSpeed() / 2)
                        << ui::FlyOut(playbackDuration_, Point{220, 0});
                    waitUntilIdle();
                    playbackInfo_->setVisibility(false);
                }
            }
        }

    private:
        Launcher::BorrowedCarousel * carousel_;
        bool repeat_ = false;
        bool shuffle_ = false;
        unique_ptr<FolderPlaylist> playlist_;
        Timer timer_;

        ui::Panel * playbackInfo_;
        ui::Image * playIcon_;
        ui::Image * pauseIcon_;
        ui::Image * shuffleIcon_;
        ui::Image * repeatIcon_;
        ui::Label * playbackTitle_;
        ui::Label * playbackDuration_;

    }; // rckid::MusicPlayer

} // namespace rckid