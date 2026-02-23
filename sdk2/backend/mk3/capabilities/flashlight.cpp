#include <platform.h>

#include <rckid/capabilities/flashlight.h>

namespace rckid {

    namespace {

        constexpr gpio::Pin FLASHLIGHT_PIN = 47;
        constexpr uint32_t PWM_SLICE = 11;

        Flashlight flashlight;

    }

    Flashlight * Flashlight::instance() {
        return & flashlight;
    }

    void Flashlight::setActive(bool value) {
        active_ = value;
        UNIMPLEMENTED;
    }

    void Flashlight::setBrightness(uint8_t value) {
        if (brightness_ == value)
            return;
        brightness_ = value;
        if (brightness_ == 0) {
            gpio_set_function(FLASHLIGHT_PIN, GPIO_FUNC_SIO);
        } else if (brightness_ == 255) {
            gpio_set_function(FLASHLIGHT_PIN, GPIO_FUNC_SIO);
            gpio::outputHigh(FLASHLIGHT_PIN);
        } else {
            gpio_set_function(FLASHLIGHT_PIN, GPIO_FUNC_PWM);
            pwm_set_wrap(PWM_SLICE, 256);
            pwm_set_clkdiv(PWM_SLICE, 64.0f);
            pwm_set_chan_level(PWM_SLICE, PWM_CHAN_B, brightness_);
            pwm_set_enabled(PWM_SLICE, true);
        }
    }

} // namespace rckid

