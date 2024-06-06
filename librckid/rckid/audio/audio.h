# pragma once

namespace rckid {

    class AudioStream;

    /** Encapsulates the audio playback and recording low lever drivers. 

        Audio output is done via two PWM channels. At the maximum clock frequency of 200MHz, this gives us 12bit sound up to 44.1kHz with a bit of a headroom. 

        Recording a microphone is done using a pio state machine for generating the PDM clock and a PWM module for (1) generating the periodic sampling interrupt at 8 or 16kHz and (b) counting the number of pulses from the microphone in the period. 
     */
    class audio {
    public:
        /** Returns true if playing through headphones, i.e. audio is enabled and headphones are connected. 
         */
        static bool headphonesActive();
        static unsigned volume() { return volume_; }
        static void setVolume(unsigned value) { volume_ = value; }

        /** Starts playback of given audio stream. The stream is not owned and its cleanup is up to the caller after the playback is done. 
         */
        static void play(AudioStream * stream);

        /** Stops the audio playback, forgetting the current audio stream, if any.
         */
        static void stop();

        /** Pauses the audio playback or recording. 
         */
        static void pause();

    private:
        friend void irqDMADone_();
        friend void initialize();

        static void initialize();
        static void configurePlaybackDMA(int dma, int other, uint16_t const * buffer, size_t bufferSize);

        static inline AudioStream * playback_ = nullptr;
        static inline uint16_t volume_ = 15;
        static inline int dma0_;
        static inline int dma1_;
        static inline uint16_t buffer0_[RP_AUDIO_BUFFER_SIZE]; 
        static inline uint16_t buffer1_[RP_AUDIO_BUFFER_SIZE]; 
        static inline int micSm_;

    }; // class rckid::audio

} // namespace rckid