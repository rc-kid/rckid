#pragma once

#include <rckid/rckid.h>
#include <rckid/audio/decoder_stream.h>

namespace rckid::audio {

    class Playlist2 {
    public:
        
        virtual ~Playlist2() = default;

        virtual uint32_t size() const = 0;

        virtual unique_ptr<DecoderStream> at(uint32_t index) = 0;

        virtual String titleAt(uint32_t index) const = 0;

    }; // rckid::audio::Playlist 

    class FolderPlaylist : public Playlist2 {
    public:

        FolderPlaylist(String folder, fs::Drive drive = fs::Drive::SD): 
            drive_{drive},
            folder_{std::move(folder)}
        {
            // TODO scan the folder and find all audio files
            fs::readFolder(folder_, drive_, [this](fs::FolderEntry const & entry) {
                if (! entry.isFolder && FileBrowser::audioFileFilter(fs::join(folder_, entry.name)))
                    files_.push_back(entry.name);
            });
        }

        uint32_t size() const override {
            return static_cast<uint32_t>(files_.size());
        }

        unique_ptr<DecoderStream> at(uint32_t index) override {
            if (index >= files_.size())
                return nullptr;
            return DecoderStream::fromFile(fs::join(folder_, files_[index]), drive_);
        }

        String titleAt(uint32_t index) const override {
            if (index >= files_.size())
                return "";
            return fs::stem(files_[index]);
        }

        uint32_t indexOf(String const & filename) const {
            for (uint32_t i = 0, e = files_.size(); i < e; ++i) {
                if (files_[i] == filename)
                    return i;
            }
            return 0;
        }

    private:
        fs::Drive drive_;
        String folder_;

        std::vector<String> files_;


    }; // 


    class PlaybackTask : public Task {
    public:

        std::function<void(uint32_t)> onTrackChanged;

        PlaybackTask(Playlist2 * playlist, uint32_t index = 0): 
            playlist_{playlist}, 
            index_{index}
        {
            indices_.reserve(playlist_->size());
            for (uint32_t i = 0, e = playlist_->size(); i < e; ++i)
                indices_.push_back(i);
        }

        ~PlaybackTask() override {
            // stop audio playback when exitting
            stop();
        }

        bool repeat() const { return repeat_; }

        void setRepeat(bool value) { repeat_ = value; }

        bool shuffle() const { return shuffle_; }

        void setShuffle(bool value) {
            shuffle_ = value;
            if (shuffle_) {
                // shuffle the indices
                // TODO
            } else {
                for (uint32_t i = 0, e = playlist_->size(); i < e; ++i)
                    indices_[i] = i;
            }
         }

        void play() {
            if (currentStream_ != nullptr)
                return;
            if (indices_.empty())   
                return;
            currentStream_ = playlist_->at(indices_[index_]);
            if (currentStream_ != nullptr) {
                t_.start();
                audio::play(currentStream_.get());
                if (onTrackChanged)
                    onTrackChanged(indices_[index_]);
            }
        }

        void next() {
            audio::stop();
            currentStream_ = nullptr;
            if (! repeat_)
                index_ = (index_ + 1) % indices_.size();
            play();
        }

        void prev() {
            audio::stop();
            currentStream_ = nullptr;
            if (! repeat_)
                index_ = index_ == 0 ? indices_.size() - 1 : index_ - 1;
            play();                
        }

        void pause() {
            if (audio::isPaused())
                return;
            audio::pause();
            t_.pause();
        }

        void resume() {
            if (! audio::isPaused())
                return;
            audio::resume();
            t_.resume();
        }

        TinyTime elapsed() const { return t_.time(); }

        String title() const {
            if (currentStream_ == nullptr)
                return "";
            return playlist_->titleAt(indices_[index_]);
        }

    protected:

        void onTick() override {
            if (currentStream_ != nullptr)
                currentStream_->update();
            // if we are done playing the current file, move to the next one, if we can
            if (!audio::isPlaying())
                next();
            else
                t_.tick();
        }

    private:

        Playlist2 * playlist_ = nullptr;
        std::vector<uint32_t> indices_;
        Timer t_;
        uint32_t index_ = 0;
        bool shuffle_ = false;
        bool repeat_ = false;

        unique_ptr<DecoderStream> currentStream_;

    }; // rckid::audio::PlaybackTask


    /** Playlist interface
     
        Interface for the playback task that allows it to move between tracks. 

        TODO the playlist can also have events from the playback task
     */
    class Playlist {
    public:

        virtual ~Playlist() = default;

        virtual unique_ptr<DecoderStream> next() = 0;

        virtual unique_ptr<DecoderStream> prev() = 0;


    protected:

    }; // rckid::audio::Playlist

    /** Audio playback task. 
     
        The task takes given playlist and plays it independently. 
     */
    class Playback : public Task {
    public:
        Playback(Playlist * playlist): playlist_{playlist} {
            currentStream_ = playlist_->next();
            if (currentStream_ != nullptr)
                play(currentStream_.get());
        }

        ~Playback() override {
            // stop audio playback when exitting
            stop();
        }

        bool next() {
            audio::stop();
            currentStream_ = playlist_->next();
            if (currentStream_ == nullptr)
                return false;
            t_.start();
            play(currentStream_.get());
            return true;
        }

        bool prev() {
            audio::stop();
            currentStream_ = playlist_->prev();
            if (currentStream_ == nullptr)
                return false;
            t_.start();
            play(currentStream_.get());
            return true;
        }

        void pause() {
            if (audio::isPaused())
                return;
            audio::pause();
            t_.pause();
        }

        void resume() {
            if (! audio::isPaused())
                return;
            audio::resume();
            t_.resume();
        }

        TinyTime elapsed() const { return t_.time(); }

    protected:
        void onTick() override {
            if (currentStream_ != nullptr)
                currentStream_->update();
            // if we are done playing the current file, move to the next one, if we can
            if (!audio::isPlaying())
                next();
            else
                t_.tick();
        }

    private:

        Playlist * playlist_ = nullptr;
        Timer t_;

        unique_ptr<DecoderStream> currentStream_;

    }; // rckid::audio::Playback

} // namespace rckid::audio