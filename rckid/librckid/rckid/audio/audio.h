# pragma once

namespace rckid {

    class AudioStream;

    /** Encapsulates the audio playback and recording low lever drivers. 

        Audio output is done via two PWM channels. At the maximum clock frequency of 200MHz, this gives us 12bit sound up to 44.1kHz with a bit of a headroom. 

        Recording a microphone is done using a pio state machine for generating the PDM clock and a PWM module for (1) generating the periodic sampling interrupt at 8 or 16kHz and (b) counting the number of pulses from the microphone in the period. 
     */
    class audio {
    public:

        enum class State {
            Off, 
            Playback, 
            Record, 
        }; // audio::State

        class OutStream {
        public:
            virtual ~OutStream() = default;

            virtual uint16_t sampleRate() { return 44100; }

            virtual void fillBuffer(uint16_t * buffer, size_t bufferSize) = 0;

        }; // audio::OutStream

        static constexpr uint16_t BaseLevel = 2048;

        /** Returns true if playing through headphones, i.e. audio is enabled and headphones are connected. 
         */
        static bool headphonesActive();
        static unsigned volume() { return volume_; }
        static void setVolume(unsigned value) { volume_ = value; }

        /** Starts playback of given audio stream. The stream is not owned and its cleanup is up to the caller after the playback is done. 
         */
        static void play(OutStream * stream);

        /** Stops the audio playback, forgetting the current audio stream, if any.
         */
        static void stop();

        /** Pauses the audio playback or recording. 
         */
        static void pause();


        /** Starts recording with given function for callback. 
         */
        static void record(std::function<void(uint16_t const *, size_t)> cb);

    private:
        friend void irqDMADone_();
        friend void initialize();

        static void initialize();
        static void irqHandler1();
        static void irqHandler2();

        static void onBufferRecorded(uint16_t * buffer, size_t size);

        /** Configures the playback DMA that reads from the buffer and writes to the output PWM, switching to th second DMA immediately to swap the buffers. 
         */
        static void configurePlaybackDMA(int dma, int other, uint16_t * buffer, size_t bufferSize);

        /** Configures the recording DMA to read from the mic PWM counter and save to the buffer memory, triggering the other DMA when done. 
         */
        static void configureRecordDMA(int dma, int other, uint16_t * buffer, size_t bufferSize);


        static inline State state_ = State::Off; 

        static inline OutStream * playback_ = nullptr;
        static inline uint16_t volume_ = 15;
        static inline int dma0_;
        static inline int dma1_;
        static inline uint16_t buffer0_[RP_AUDIO_BUFFER_SIZE]; 
        static inline uint16_t buffer1_[RP_AUDIO_BUFFER_SIZE]; 
        static inline int micSm_;
        static inline unsigned micOffset_;
        static inline uint16_t micLast_;
        static inline std::function<void(uint16_t const *, size_t)> micCallback_;

    }; // class rckid::audio

} // namespace rckid