#pragma once

#include <rckid/task.h>
#include <rckid/buffer.h>

namespace rckid::audio {

    class DecoderStream {
    public:

        virtual ~DecoderStream() = default;

        void update() {
            for (uint32_t i = 0, e = playbackBuffer_.numBuffers(); i < e; ++i) {
                Buffer<int16_t> * buffer = playbackBuffer_.nextFree();
                if (buffer == nullptr)
                    break;
                buffer->setUsed(refillSamples(buffer->data(), playbackBuffer_.size() / 2));
                playbackBuffer_.markReady(buffer);
            }
        }

    protected:
        DecoderStream(uint32_t bufferStereoSamples, uint32_t numBuffers = 8):
            playbackBuffer_{bufferStereoSamples * 2, numBuffers} {
        }

        /** The main method to refill the buffer with samples. 
         
            Given is the buffer that is larger enough to hold numStereoSamples stereo samples (a stereo sample is 2 int16_t). The function should fill the buffer and return the number of stereo samples written to the buffer.
         */
        virtual uint32_t refillSamples(int16_t * buffer, uint32_t numStereoSamples) = 0;


    private:

        MultiBuffer<int16_t> playbackBuffer_;

    }; // rckid::audio::DecoderStream

} // namespace rckid::audio