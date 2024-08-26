#pragma once

#include "../assets/easing_tables.h"

namespace rckid {

    enum class Easing : uint8_t {
        Linear, 
        Sin, 
        Cos, 
    }; // rckid::Easing

    template<typename T>
    inline T easeInRange(T start, T end, int promille, Easing e = Easing::Linear) {
        //if (end < start)
        //    promille *= -1;
        switch (e) {
            default: // a bit of defensive programming
            case Easing::Linear:
                return (end - start) * promille / 1000 + start;
            case Easing::Sin:
                // convert promille to 0..511
                promille = promille * 511 / 1000;
                if (promille < 255)
                    return static_cast<T>((end - start) * SineTable[promille] / 131072 + start);
                else
                    return static_cast<T>((end - start) * (131072 - SineTable[511 - promille]) / 131072 + start);
            case Easing::Cos:
                if (promille == 1000)
                    return end;
                // convert promille to 0..511
                promille = promille * 511 / 1000;
                if (promille < 255) 
                    return static_cast<T>((end - start) * (65536 - SineTable[255 - promille]) / 131072 + start);
                else 
                    return static_cast<T>((end - start) * (65536 + SineTable[promille - 256]) / 131072 + start);
        }
    }

} // namespace rckid