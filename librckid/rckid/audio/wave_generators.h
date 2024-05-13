#pragma once

#include "wave_tables.h"

namespace rckid {

    /** We only use 24 bits for the wave resolution, so that uint32_t allows us to keep multiple second duration. 
     */
    constexpr uint32_t WaveBitDepth = 32;
    constexpr uint32_t WavePeriod = std::numeric_limits<uint32_t>::max();
    constexpr uint16_t WaveMidLine = 2048;

    /** The simplest, square wave generator. 
     */
    class SquareWave {
    public:
        static uint16_t valueAt(uint32_t t, uint16_t amp) { 
            return WaveMidLine + ((t >= WavePeriod / 2) ? amp : -amp); 
        }
    };

    class SineWave {
    public:
        static uint16_t valueAt(uint32_t t, uint16_t amp) {
            static_assert(sizeof(SineTable) / sizeof(uint16_t) == 256);
            int value = 0;
            t = (t >> (WaveBitDepth - 10));
            switch (t >> 8) {
                case 0: // increasing from 0 to max, default sine table
                    value = SineTable[t & 0xff];
                    break;
                case 1: // decreasing from amp to 0 
                    value = SineTable[255 - (t & 0xff)];
                    break;
                case 2: // decreasing from 0 to - amp
                    value = - SineTable[t & 0xff];
                    break;
                case 3: // increasing from -amp to 0
                    value = - SineTable[255 - (t & 0xff)];
                    break;
            }
            return WaveMidLine + value * amp / 65536;
        }
    };

    class SawToothWave {
    public:
        static uint16_t valueAt(uint32_t t, uint16_t amp) {
            return WaveMidLine - amp + (2 * amp) * (t >> (WaveBitDepth - 16)) / 65536;
        }
    };

    class WhiteNoise {

    };


}; // namespace rckid