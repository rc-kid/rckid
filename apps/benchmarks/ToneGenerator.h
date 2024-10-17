#pragma once

#include "benchmark.h"

namespace rckid {

    class ToneGenerator {
    public:
        void run(Canvas<ColorRGB> & g_) {
            g_.text(0, 20) << 
                "Tone generation measurements [us]\n" <<
                "Sine 8khz:      " << measure(Tone::Waveform::Sine, 8000) << "\n" <<
                "Sine 16khz:     " << measure(Tone::Waveform::Sine, 16000) << "\n" <<
                "Sine 44.1khz:   " << measure(Tone::Waveform::Sine, 44100) << "\n" <<
                "Square 8khz:    " << measure(Tone::Waveform::Square, 8000) << "\n" <<
                "Square 16khz:   " << measure(Tone::Waveform::Square, 16000) << "\n" <<
                "Square 44.1khz: " << measure(Tone::Waveform::Square, 44100);
        }

        uint32_t measure(Tone::Waveform kind, uint32_t sampleRate) {
            uint32_t time;
            MEASURE_TIME(time, {
                Tone t;
                volatile int16_t res = 0;
                t.setWaveform(kind);
                t.setFrequency(440, 1000, sampleRate);
                for (uint32_t i = 0; i < sampleRate; ++i)
                    res += t.next();
            });
            return time;
        }


    }; 


} // namespace rckid 