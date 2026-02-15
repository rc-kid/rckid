#pragma once

#include <platform.h>

#include <rckid/task.h>
#include <rckid/audio/decoder_stream.h>

namespace rckid::audio {

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
        }

        bool next() {
            UNIMPLEMENTED;
        }

        bool prev() {
            UNIMPLEMENTED;
        }

    private:

        Playlist * playlist_ = nullptr;

        unique_ptr<DecoderStream> currentStream_;

    }; // rckid::audio::Playback


} // namespace rckid::audio