#pragma once

#include <cstdint>


namespace rckid {

    /** A very simple, arduino-like square wave tone generator. 
     */
    class Tone {
    public:
        enum class Kind {
            Square, 
            Triangle,
            Sine, 
        }; 
        void play(unsigned frequency, unsigned volume = 10) {
            volume_ = volume;
            ticks_ = 44100 / frequency; 
            cnt_ = 0;
        }

        /** Fills the provided buffer with square wave of the current frequency. 
        */
        void fillBuffer(uint16_t * buffer, size_t stereoSamples) {
            while (stereoSamples-- != 0) {
                buffer[0] = 128;
                buffer[1] = getNextValue(Kind::Square);
                cnt_ = (cnt_ + 1) % ticks_;
                buffer += 2;
            }
        }

        uint16_t getNextValue(Kind kind) {
            switch (kind) {
                case Kind::Square:
                    return (cnt_ <= ticks_ / 2) ? (128 + volume_) : (128 - volume_); 
                case Kind::Triangle:
                    return 128 - volume_ + (volume_ * 2 * cnt_ / ticks_);
                default:
                    // this is really an error
                    return 128;
            }
        }

    private:

        unsigned volume_;
        unsigned ticks_;
        unsigned cnt_;
        

    }; // rckid::Tone

} // namespace rckid