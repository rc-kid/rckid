#pragma once

#include <rckid/rckid.h>
#include <rckid/fixedint.h>

namespace rckid {

    /** Virtual analog joystick.

        RCKid has no analog stick, but games often want an analog-ish input instead of the on/off of the dpad. The joystick provides one by driving each axis either from the dpad (in which case holding a direction accelerates from 1 towards the maximum), or from the accelerometer (in which case the tilt of the device is mapped onto the axis range), or from both, which is the default - the dpad takes over whenever it is held and the accelerometer drives the axis otherwise.
     */
    class Joystick {
    public:

        enum class Mode {
            Dpad,
            Accelerometer,
            Both,
        };

        /** Single joystick axis.

            The value of the axis is a number from -127 to 128, which corresponds to the full deflection in either direction.
         */
        class Axis {
        public:

            /** What drives the axis.
             */
            Mode mode = Mode::Both;

            /** Accelerometer readouts that correspond to the full deflection of the axis and to the dead zone around its center.
             */
            //@{
            int16_t min = -15000;
            int16_t max = 15000;
            int16_t centerMin = -500;
            int16_t centerMax = 500;
            //@}

            /** How fast the axis accelerates towards its maximum when driven by the dpad.
             */
            FixedInt acceleration{1, 0x1};

            /** Returns the current value of the axis (-127 to 128).
             */
            int32_t value() const { return value_.round(); }

            /** Returns the current value of the axis clamped to the given range.
             */
            int32_t value(int32_t minValue, int32_t maxValue) const {
                int32_t result = value_.round();
                return (result < minValue) ? minValue : ((result > maxValue) ? maxValue : result);
            }

            void reset() { value_ = 0; }

            void update(bool btnMinus, bool btnPlus, int16_t accelValue) {
                if (mode != Mode::Accelerometer && (btnMinus != btnPlus)) {
                    if (btnMinus) {
                        if (value_ >= 0)
                            value_ = -1;
                        else
                            value_ = (value_ * acceleration).clamp(-127, 128);
                    } else {
                        if (value_ <= 0)
                            value_ = 1;
                        else
                            value_ = (value_ * acceleration).clamp(-127, 128);
                    }
                    return;
                }
                if (mode == Mode::Dpad) {
                    value_ = 0;
                    return;
                }
                if (accelValue <= centerMin) {
                    if (accelValue < min)
                        accelValue = min;
                    value_ = FixedInt{(accelValue - centerMin) * 127} / - (min - centerMin);
                } else if (accelValue >= centerMax) {
                    if (accelValue > max)
                        accelValue = max;
                    value_ = FixedInt{(accelValue - centerMax) * 128} / (max - centerMax);
                } else {
                    value_ = 0;
                }
            }

        private:
            FixedInt value_;

        }; // rckid::Joystick::Axis

        Axis x;
        Axis y;

        Joystick() = default;

        explicit Joystick(Mode mode) {
            setMode(mode);
        }

        void setMode(Mode mode) {
            x.mode = mode;
            y.mode = mode;
        }

        void reset() {
            x.reset();
            y.reset();
        }

        /** Updates both axes. Should be called once per frame.
         */
        void update() {
            Point3D a = accel();
            x.update(btnDown(Btn::Left), btnDown(Btn::Right), static_cast<int16_t>(a.x));
            y.update(btnDown(Btn::Down), btnDown(Btn::Up), static_cast<int16_t>(a.y));
        }

    }; // rckid::Joystick

} // namespace rckid
