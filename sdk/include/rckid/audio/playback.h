#pragma once

#include <rckid/rckid.h>
#include <rckid/audio/decoder_stream.h>

namespace rckid::audio {

    /** Playlist interface.
     
        A playlist is a collection of audio files. The playlist is generic interface to any such collection and provides the API for knowing the size of the playlist, getting the audio stream for a given index and getting the track title for given index. 
     */
    class Playlist {
    public:
        
        virtual ~Playlist() = default;

        /** Returns the size of the playlist (number of tracks).
         */
        virtual uint32_t size() const = 0;

        /** Returns the audio stream for the given index.
         */
        virtual unique_ptr<DecoderStream> at(uint32_t index) = 0;

        /** Returns the title of the track at the given index.
         
            This is the fallback title in case the audio stream does not provide any metadata for the track.
         */
        virtual String titleAt(uint32_t index) const = 0;

    }; // rckid::audio::Playlist 

    /** Playlist created from a folder of audio files.
     
        Audio playlist created from all audio files in a given folder (non-recursive). File names without the extension are the default track titles.
     */
    class FolderPlaylist : public Playlist {
    public:

        FolderPlaylist(String folder, fs::Drive drive = fs::Drive::SD): 
            drive_{drive},
            folder_{std::move(folder)}
        {
            // scan the folder and find all audio files
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

    }; // rckid::audio::FolderPlaylist


    /** Audio playback task.
     
        Given an audio playlist, the task plays it in succession and supports pausing, resuming and track control. The task is designed to be run in the background and will automatically play the next track when the current one finishes. It also supports shuffle and repeat modes. 

        No uservisible controls are available (for a simple user interface, see the audio::Player app instead).
     */
    class PlaybackTask : public Task {
    public:

        /** Event triggered when the track changes. 
         
            Has the playlist index of the new track as the parameter.
         */
        std::function<void(uint32_t)> onTrackChanged;

        PlaybackTask(Playlist * playlist, uint32_t index = 0): 
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
                for (uint32_t i = 0, e = indices_.size(); i < e; ++i) {
                    uint32_t j = cpu::random() % e;
                    std::swap(indices_[i], indices_[j]);
                }
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
            if (indices_.empty())
                return;
            if (index_ >= indices_.size())
                index_ = 0;
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
            if (indices_.empty())
                return;
            currentStream_ = nullptr;
            if (! repeat_)
                index_ = (index_ + 1) % indices_.size();
            play();
        }

        void prev() {
            audio::stop();
            if (indices_.empty())
                return;
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

        Playlist * playlist_ = nullptr;
        std::vector<uint32_t> indices_;
        Timer t_;
        uint32_t index_ = 0;
        bool shuffle_ = false;
        bool repeat_ = false;

        unique_ptr<DecoderStream> currentStream_;

    }; // rckid::audio::PlaybackTask


} // namespace rckid::audio