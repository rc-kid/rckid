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


    /** Base class for audio players of differeht formats. 
     */
    class AudioStream {
    public:
        virtual ~AudioStream() {
            delete playbackBuffer_;
        }

        virtual uint32_t refillSamples(int16_t * buffer, uint32_t numSamples) = 0;

        virtual uint32_t sampleRate() const = 0;

        /** To be called periodically in the main loop, checks there is need for more audio data to be generated and calls the refill samples function accordingly.
         */
        void update() {
            if (refill_ != nullptr) {
                *refillSize_ = refillSamples(refill_, *refillSize_);
                refill_ = nullptr;
            }
        }

        uint32_t underflowCount() const { return underflowCount_; }
   
    protected:

        AudioStream(uint32_t bufferSize) {
            playbackBuffer_ = new DoubleBuffer<int16_t>{bufferSize};
        }

    private:

        friend void audioPlay(AudioStream & stream);
        uint32_t underflowCount_ = 0;

        
        DoubleBuffer<int16_t> * playbackBuffer_ = nullptr;
        uint32_t * refillSize_ = nullptr;
        int16_t * refill_ = nullptr;
    }; // rckid::AudioPlayer

    inline void audioPlay(AudioStream & stream) {
        uint32_t sampleRate = stream.sampleRate();

        audioPlay(sampleRate, [& stream](int16_t * & buffer, uint32_t & size) {
            
            if (buffer == nullptr) {
                size = stream.refillSamples(stream.playbackBuffer_->front(), stream.playbackBuffer_->size() / 2);
                buffer = stream.playbackBuffer_->front();
                stream.playbackBuffer_->swap();
            } else {
                if (stream.refill_ != nullptr)
                    ++stream.underflowCount_;
                stream.refill_ = buffer;
                stream.refillSize_ = & size;
            }
        });
    }



} // namespace rckid