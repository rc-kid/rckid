#pragma once

#include <rckid/ui/app.h>
#include <rckid/apps/launcher.h>
#include <rckid/apps/file_browser.h>

#include <rckid/audio/mp3.h>

namespace rckid {

    /** Simple music player. 
     */
    class MusicPlayer : public ui::App<void> {
    public:

        String name() const override { return "Music"; }

        MusicPlayer() {
            using namespace ui;
            carousel_ = addChild(new Launcher::BorrowedCarousel());
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
            }

            void moveToPrev() {
                if (player_->shuffle_)
                    index_ = rand() % player_->carousel_->menu()->size();
                else if (! player_->repeat_)
                    index_ = player_->carousel_->prevIndex();
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
                }, "/files/music", fs::Drive::SD); });
        }

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(carousel_);
        }

        void loop() override {
            ui::App<void>::loop();
            // when file is selected, simply ensure that the playlist is created and start the playback. This will call the action via next
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                ASSERT(playlist_ == nullptr);
                playlist_ = std::make_unique<FolderPlaylist>(this);
                playlist_->start();
            }
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                ASSERT(carousel_->atRoot());
                // TODO terminate music, etc
                exit();
            }
        }

    private:
        Launcher::BorrowedCarousel * carousel_;
        bool repeat_ = false;
        bool shuffle_ = false;
        unique_ptr<FolderPlaylist> playlist_;
    }; // rckid::MusicPlayer

} // namespace rckid