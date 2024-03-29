#pragma once

/** Interpolation modes - see interpolate() for mode details. 
 
    NOTE: Not to be confused with the RP2040's interpolator HW unit. 
*/
enum class Interpolation {
    Linear, 
    Sin, 
    Cos, 
}; // rckid::Interpolation

/// Sin table for very imprecise integer only sin calculations. 
static constexpr uint8_t SinTable[] = { 
    0,  3,  6,  9,  12, 15, 18, 21, 24, 27, 
    30, 33, 36, 39, 42, 45, 48, 50, 53, 56,
    58, 61, 63, 66, 68, 70, 72, 75, 77, 79,
    80, 82, 84, 86, 87, 89, 90, 91, 92, 94,
    95, 96, 96, 97, 98, 98, 99, 99, 99, 100
};

/** Interpolates value between start and end (both inclusive) using the given promille value and interpolation algorithm. 
 
    - Interpolation::Linear will provide linear increase from start to end wrt promille
    - Interpolation::Sin will ajust the interpolation by sin function, i.e. fast start, slow middle, fast end
    - Interpolation::Cos uses the cos function to adjust the value, i.e. show start, fast middle and slow end, which corresponds to the natural accelerate - brake movement of things 
*/
template<typename T> 
inline T interpolate(T start, T end, int promille, Interpolation i = Interpolation::Linear) {
    if (end < start)
        promille *= -1;
    switch (i) {
        default: // a bit of defensive programming
        case Interpolation::Linear:
            return (end - start) * promille / 1000 + start;
        case Interpolation::Sin:
            if (promille <= 500)
                return static_cast<T>((end - start) * SinTable[(promille + 5) / 10] / 200 + start);
            else
                return static_cast<T>((end - start) * (200 - SinTable[sizeof(SinTable) - 1 - (promille - 500 + 5) / 10]) / 200 + start);
        case Interpolation::Cos:
            if (promille <= 505)
                return static_cast<T>((end - start) * (100 - SinTable[sizeof(SinTable) - 1 - ((promille + 5) / 10)]) / 200 + start);
            else
                return static_cast<T>((end - start) * (100 + SinTable[(promille - 510 + 5) / 10]) / 200 + start);
    }
}
