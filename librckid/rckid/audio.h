#pragma once

#include "rckid.h"

namespace rckid {

    enum class SampleRate : uint16_t {
        kHz8 = 8000, 
        kHz16 = 16000,
        kHz44_1 = 44100,
        kHz48 = 48000,
    };

    /** Audio driver. 
    */
    class Audio {
    public:

        /** Callback for audio playback takes the beginning of a buffer and the length of the buffer in 2 byte elements. When called, the callback must populate the buffer with the new audio values. 
        */
        using CallbackPlay = std::function<void(uint16_t *, size_t)>;


        static void initialize();

        static bool headphones() { return Device::state_.status.headphones(); }
        
        static bool audioEnabled() { return Device::state_.status.audioEnabled(); }

        static void setAudioEnabled(bool enabled = true) {
            if (enabled) 
                Device::sendCommand(cmd::AudioEnabled{});
            else
                Device::sendCommand(cmd::AudioDisabled{});
        }

        /** Buffer size (num elements, i.e. stereoSamples * 2) */
        static void startPlayback(SampleRate rate, uint16_t * buffer, size_t bufferSize, CallbackPlay cb);

        static void stopPlayback();

        static void startRecording(SampleRate rate); 

        static void stopRecording();

    private:

        friend void yield();

        /** Called by the rckid's sdk as part of each frame to check if the playback or recording buffers have to be processed. 
         */
        static void processEvents();

        static void configureDMA(int dma, int other, uint16_t const * bufferStart, size_t bufferSamples);


#if (RP_PIN_PWM_RIGHT == 24 && RP_PIN_PWM_LEFT == 25)
        static constexpr unsigned PWM_SLICE = 4; 
#elif (RP_PIN_PWM_RIGHT == 14 && RP_PIN_PWM_LEFT == 15) // RCKID_AUDIO_DEBUG
        static constexpr unsigned PWM_SLICE = 7; 
#else
        #error "Unsupported audio PWM pins"
#endif

        // slice 5 corresponds to pins 10,11, 26 and 27 which are not available as cartridge GPIO pins
        static constexpr unsigned TIMER_SLICE = 5;

        static void setSampleRate(SampleRate rate) { setSampleRate(static_cast<uint16_t>(rate)); }

        static void setSampleRate(uint16_t rate);

        static void irqDMADone();

        static inline int dma0_;
        static inline int dma1_;    

        static inline uint16_t * buffer_;

        static inline size_t bufferSize_;

        static inline CallbackPlay cbPlay_;

        static constexpr unsigned BUFFER_INDEX = 1 << 0;
        static constexpr unsigned CALLBACK = 1 << 1;
        static constexpr unsigned PLAYBACK = 1 << 2;
        static constexpr unsigned RECORDING = 1 << 3;

        static inline unsigned volatile status_;

    }; // rckid::Audio

} // namespace rckid