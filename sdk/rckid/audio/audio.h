#pragma once

#include "../rckid.h"
#include "../utils/fixedint.h"

namespace rckid {

    namespace audio {

        constexpr int16_t min = -32768;
        constexpr int16_t max = 32767;

        /** Given a frequency, returns the corresponding period in microseconds (us) as a fixed integer.
         */
        inline FixedInt frequencyToPeriodUs(uint32_t freq) {
            freq = 1000000 * 16 / freq;
            return FixedInt{static_cast<int>(freq >> 4), freq & 0xf};
        }

        /** Converts mono buffer into stereo one, duplicating all samples in it in place (from last to first). 
         */
        inline void convertToStereo(int16_t * buffer, uint32_t numSamples) {
            for (; numSamples > 0; --numSamples) {
                int16_t x = buffer[numSamples - 1];
                buffer[numSamples * 2 - 2] = x;
                buffer[numSamples * 2 - 1] = x;
            }
            //buffer[1] = buffer[0];
        }

    } // namespace audio


    class AudioStream;
    void audioPlay(AudioStream & stream);


    /** Base class for audio players of different formats. 
     */
    class AudioStream {
    public:

        static AudioStream * fromFile(String const & path, uint32_t numBuffers = 4);

        virtual ~AudioStream() {
            delete playbackBuffer_;
        }

        virtual String name() const = 0;

        virtual uint32_t refillSamples(int16_t * buffer, uint32_t numSamples) = 0;

        virtual uint32_t sampleRate() const = 0;

        /** To be called periodically in the main loop, checks there is need for more audio data to be generated and calls the refill samples function accordingly.
         */
        void update() {
            for (uint32_t i = 0, e = playbackBuffer_->numBuffers(); i < e; ++i) {
                CountedBuffer<int16_t> * buffer = playbackBuffer_->nextFree();
                if (buffer == nullptr)
                    break;
                buffer->size = refillSamples(buffer->data, buffer->size / 2);
                playbackBuffer_->markReady(buffer);
            }
        }

        uint32_t underflowCount() const { return underflowCount_; }
   
    protected:

        AudioStream(uint32_t bufferSize, uint32_t numBuffers = 4) {
            playbackBuffer_ = new MultiBuffer<int16_t>{bufferSize, numBuffers};
        }

    private:

        friend void audioPlay(AudioStream & stream);
        uint32_t underflowCount_ = 0;

        
        MultiBuffer<int16_t> * playbackBuffer_ = nullptr;
    }; // rckid::AudioStream

    inline void audioPlay(AudioStream & stream) {
        uint32_t sampleRate = stream.sampleRate();

        audioPlay(sampleRate, [& stream](int16_t * & buffer, uint32_t & size) {
            
            if (buffer == nullptr) {
                CountedBuffer<int16_t> * b = stream.playbackBuffer_->nextFree();
                size = stream.refillSamples(b->data, b->size / 2);
                buffer = b->data;
            } else {
                CountedBuffer<int16_t> * b = stream.playbackBuffer_->nextReady();
                if (b == nullptr) {
                    ++stream.underflowCount_;
                } else {
                    stream.playbackBuffer_->markFree(buffer);
                    buffer = b->data;
                    size = b->size;
                }
            }
        });
    }



} // namespace rckid