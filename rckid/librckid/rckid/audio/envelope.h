#pragma once

#include "rckid/utils/easing.h"
namespace rckid {

    /** The ADSR Envelope specification. 
     
        Attack, Decay and Release are unitless time measures, while Sustain is amplitude. 
     */
    class ADSREnvelope {
    public:
        uint16_t attack;
        uint16_t decay;
        uint16_t release;
        uint8_t sustain;
        Easing easing;

        int16_t modulate(int16_t value, uint16_t t, uint16_t tMax) {
            if (tMax - t <= release) {
                t = release - (tMax - t);
                return value * easeInRange(sustain * 10, 0, t * 1000 / release) / 1000;
            }
            if (t <= attack)
                return value * easeInRange(0, 1000, t * 1000 / attack, easing) / 1000;
            if (t <= attack + decay) {
                t -= attack;
                return value * easeInRange(1000, sustain * 10, t * 1000 / decay, easing) / 1000;
            }
            return value * sustain / 100;
        }

    } __attribute__((packed)); // rckid::ADSREnvelope

} // namespace rckid