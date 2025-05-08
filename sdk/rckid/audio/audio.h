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
                int16_t x = buffer[numSamples];
                buffer[numSamples * 2] = x;
                buffer[numSamples * 2 + 1] = x;
            }
            buffer[1] = buffer[0];
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
    
    protected:

        AudioStream(uint32_t bufferSize, Allocator & a) {
            playbackBuffer_ = new (a.alloc<DoubleBuffer<int16_t>>()) DoubleBuffer<int16_t>(bufferSize, a);
        }
        
    private:

        friend void audioPlay(AudioStream & stream);

        
        DoubleBuffer<int16_t> * playbackBuffer_ = nullptr;

    }; // rckid::AudioPlayr

    inline void audioPlay(AudioStream & stream) {
        uint32_t sampleRate = stream.sampleRate();
        audioPlay(*(stream.playbackBuffer_), sampleRate, [& stream](int16_t * buffer, uint32_t samples) {
            return stream.refillSamples(buffer, samples);
        });
    }



} // namespace rckid