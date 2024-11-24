#pragma once

namespace rckid::audio {

    enum class Channels {
        Mono = 1, 
        Stereo = 2,
    }; 

    enum class SampleRate {
        khz8, 
        khz12,
        khz16,
        khz24,
        khz44_1,
        khz48, 
    };

} // namespace rckid::audio