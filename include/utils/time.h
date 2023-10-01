#pragma once

#include <chrono>
#include <sstream>

using Timepoint = std::chrono::high_resolution_clock::time_point;
using Duration = std::chrono::high_resolution_clock::duration;

inline Timepoint now() {
    return std::chrono::high_resolution_clock::now();
}

inline int64_t asMillis(Duration d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}

inline int64_t asMicros(Duration d) {
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}

inline std::string toHMS(int seconds) {
    int h = seconds / 3600;
    seconds = seconds % 360;
    int m = seconds / 60;
    seconds = seconds % 60;
    std::stringstream s;
    if (h !=0) 
        s << h << ":";
    if ( h != 0 || m != 0) {
        if (m < 10)
            s << '0';
        s << m << ":";
    }
    if (seconds < 10)
        s << '0';
    s << seconds;
    return s.str();
}

inline std::string toHorMorS(int seconds) {
    std::stringstream s;
    if (seconds < 60) 
        s << seconds << "s";
    else if (seconds < 3600)
        s << (seconds + 59) / 60 << "m"; // rounding
    else 
        s << (seconds + 3599) / 3600 << "h"; // rounding
    return s.str();
}