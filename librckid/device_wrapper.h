#pragma once

#include "bmi160.h"
namespace rckid {


    class DeviceWrapper {

        friend class BaseApp;

        friend void initialize();
        friend void yield();
        
        friend bool down(Btn b) { return btnDown(b, state_.state); }
        friend bool pressed(Btn b) { return btnDown(b, state_.state) && ! btnDown(b, lastState_); }
        friend bool released(Btn b) { return !btnDown(b, state_.state) && btnDown(b, lastState_); }
        friend int16_t accelX() { return aState_.accelX; }
        friend int16_t accelY() { return aState_.accelY; }
        friend int16_t accelZ() { return aState_.accelZ; }
        friend int16_t gyroX() { return aState_.gyroX; }
        friend int16_t gyroY() { return aState_.gyroY; }
        friend int16_t gyroZ() { return aState_.gyroZ; }
        friend uint16_t lightAmbient() { return lightALS_; }
        friend uint16_t lightUV() { return lightUV_; }
        friend unsigned tempAvr() { return state_.state.temp(); }

        friend void setBrightness(uint8_t brightness) { DeviceWrapper::sendCommand(cmd::SetBrightness(brightness)); }
        friend void disableLEDs() { DeviceWrapper::sendCommand(cmd::RGBOff{}); }
        friend void setButtonEffect(Btn btn, RGBEffect effect);
        friend void setButtonsEffects(RGBEffect a, RGBEffect b, RGBEffect dpad, RGBEffect sel, RGBEffect start);
        friend void setRumbler(RumblerEffect effect) { DeviceWrapper::sendCommand(cmd::Rumbler{effect}); }

        friend void powerOff();
        friend bool charging() { return state_.state.charging(); }
        friend bool dcPower() { return state_.state.dcPower(); }
        friend unsigned vcc() { return state_.state.vcc(); }

    private:

        friend void irqI2CDone_();

        template<typename T>
        static void sendCommand(T const & cmd) {
            /// TODO: ensure T is a command
            i2c_write_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t const *) & cmd, sizeof(T), false);
        }    

        static bool btnDown(Btn btn, State const & state) {
            switch (btn) {
                case Btn::Left:
                    return state.btnLeft();
                case Btn::Right:
                    return state.btnRight();
                case Btn::Up:
                    return state.btnUp();
                case Btn::Down:
                    return state.btnDown();
                case Btn::A:
                    return state.btnA();
                case Btn::B:
                    return state.btnB();
                case Btn::Select:
                    return state.btnSel();
                case Btn::Start:
                    return state.btnStart();
                case Btn::VolumeUp:
                    return state.btnVolUp();
                case Btn::VolumeDown:
                    return state.btnVolDown();
                case Btn::Home:
                    return state.btnHome();
                default:
                    // unreachable
                    return false;
            }
        }

        static void waitTickDone();

        // device & sensors state
        static inline DeviceState state_;
        static inline State lastState_;
        static inline platform::BMI160::State aState_;
        static inline uint16_t lightALS_ = 0;
        static inline uint16_t lightUV_ = 0;

    }; // rckid::Device



    inline void setButtonEffect(Btn btn, RGBEffect effect) {
        uint8_t index;
        switch (btn) {
            case Btn::B:
                index = 0;
                break;
            case Btn::A:
                index = 1;
                break;
            case Btn::Left:
            case Btn::Right:
            case Btn::Up:
            case Btn::Down:
                index = 3;
                break;
            case Btn::Select:
                index = 4;
                break;
            case Btn::Start:
                index = 5;
                break;
            default:
                // TODO can't really do anything here
                return;
        }
        DeviceWrapper::sendCommand(cmd::SetRGBEffect{index, effect});
    }

    inline void setButtonsEffects(RGBEffect a, RGBEffect b, RGBEffect dpad, RGBEffect sel, RGBEffect start) {
        DeviceWrapper::sendCommand(cmd::SetRGBEffects{a, b, dpad, sel, start});
    }


} // namespace rckid