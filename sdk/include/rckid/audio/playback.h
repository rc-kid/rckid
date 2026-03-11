#pragma once

#include <rckid/rckid.h>
#include <rckid/audio/decoder_stream.h>

namespace rckid::audio {

    /** Playlist interface
     
        Interface for the playback task that allows it to move between tracks. 

        TODO the playlist can also have events from the playback task
     */
    class Playlist {
    public:

        virtual ~Playlist() = default;

        virtual unique_ptr<DecoderStream> next() = 0;

        virtual unique_ptr<DecoderStream> prev() = 0;

    }; // rckid::audio::Playlist

    /** Audio playback task. 
     
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
            currentStream_ = playlist_->next();
            if (currentStream_ == nullptr)
                return false;
            play(currentStream_.get());
            return true;
        }

        bool prev() {
            currentStream_ = playlist_->prev();
            if (currentStream_ == nullptr)
                return false;
            play(currentStream_.get());
            return true;
        }

    protected:
        void onTick() override {
            if (currentStream_ != nullptr)
                currentStream_->update();
            // if we are done playing the current file, move to the next one, if we can
            if (!audio::isPlaying())
                next();
        }

    private:

        Playlist * playlist_ = nullptr;

        unique_ptr<DecoderStream> currentStream_;

    }; // rckid::audio::Playback

} // namespace rckid::audio