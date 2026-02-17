#pragma once

#include <platform.h>

#include <rckid/hal.h>
#include <rckid/task.h>
#include <rckid/audio/decoder_stream.h>

namespace rckid::audio {

    using Callback = hal::audio::Callback;

    inline void play(uint32_t sampleRate, Callback cb) { hal::audio::play(sampleRate, cb); }

    inline void play(DecoderStream * stream) {
        // ensure the stream's playback buffer is filled
        stream->update();
        // start the playback
        play(stream->sampleRate(), [stream](int16_t * & buffer, uint32_t & stereoSamples) {
            stream->callback(buffer, stereoSamples);
        });
    }

    inline void recordMic(uint32_t sampleRate, Callback cb) { hal::audio::recordMic(sampleRate, cb); }

    inline void recordLineIn(uint32_t sampleRate, Callback cb) { hal::audio::recordLineIn(sampleRate, cb); }

    inline void pause() { hal::audio::pause(); }

    inline void resume() { hal::audio::resume(); }

    inline void stop() { hal::audio::stop(); }

    inline bool isPlaying() { return hal::audio::isPlaying(); }

    inline bool isRecording() { return hal::audio::isRecording(); }

    inline bool isPaused() { return hal::audio::isPaused(); }

    /** Converts mono buffer into stereo one, duplicating all samples in it in place.
     
        Takes audio buffer and number of mono samples stored in it and expands the mono samples to stereo ones, duplicating each mono sample int two stereo samples in place. The caller must ensure the buffer is large enough to hold the resulting stereo samples. 
     */
    inline void convertToStereo(int16_t * buffer, uint32_t numSamples) {
        for (; numSamples > 0; --numSamples) {
            int16_t x = buffer[numSamples - 1];
            buffer[numSamples * 2 - 2] = x;
            buffer[numSamples * 2 - 1] = x;
        }
    }

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
            UNIMPLEMENTED;
        }

        bool prev() {
            UNIMPLEMENTED;
        }

    protected:
        void onTick() override {
            if (currentStream_ != nullptr)
                currentStream_->update();
        }

    private:

        Playlist * playlist_ = nullptr;

        unique_ptr<DecoderStream> currentStream_;

    }; // rckid::audio::Playback


} // namespace rckid::audio