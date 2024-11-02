#include "rckid.h"

namespace rckid {

    namespace {
        struct JoystickAxis {
            bool useDpad = true;
            int16_t min = -15000;
            int16_t max = 15000;
            int16_t centerMin = - 500;
            int16_t centerMax = 500;
            FixedInt accel{1, 0x1};
            FixedInt value;

            void update(bool btnMinus, bool btnPlus, int16_t accelValue) {
                if (useDpad) {
                    if (btnMinus == btnPlus) {
                        value = 0;
                    } else if (btnMinus) {
                        if (value >= 0)
                            value = -1;
                        else 
                           value = (value * accel).clamp(-127, 128);
                    } else if (btnPlus) {
                        if (value <= 0)
                            value = 1;
                        else
                           value = (value * accel).clamp(-127, 128);
                    }
                } else {
                    if (accelValue <= centerMin) {
                        if (accelValue < min)
                            accelValue = min;
                        value = FixedInt{(accelValue - centerMin) * 127} / - (min - centerMin);
                    } else if (accelValue >= centerMax) {
                        if (accelValue > max)
                            accelValue = max;
                        value = FixedInt{(accelValue - centerMax) * 128} / (max - centerMax);
                    } else {
                        value = 0;
                    }
                }
            }
        }; 

        JoystickAxis joyX_;
        JoystickAxis joyY_;
    }

    int8_t joystickX() { return joyX_.value.round(); }
    int8_t joystickY() { return joyY_.value.round(); }

    void joystickTick() {
        joyX_.update(btnDown(Btn::Left), btnDown(Btn::Right), accelX());
        joyY_.update(btnDown(Btn::Down), btnDown(Btn::Up), accelY());
    }

} // namespace rckid