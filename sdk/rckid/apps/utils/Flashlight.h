#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../ui/progressbar.h"
#include "../../assets/fonts/OpenDyslexic128.h"


namespace rckid {

    class Flashlight : public ui::Form<void> {
    public:

        String name() const override { return "Flashlight"; }

        Flashlight() : 
            ui::Form<void>{}
        {
            setBrightness();
            //gpio::outputHigh(47);
            //gpio::outputHigh(46);
            g_.addChild(icon_);
            g_.addChild(brightness_);
        }

        ~Flashlight() override {
            brightness_.setValue(0);
            setBrightness();
            //gpio::outputLow(47);
            //gpio::outputLow(46);
            //gpio::setAsInput(47);
            //gpio::setAsInput(46);
        }

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::Down) || btnPressed(Btn::B))
                exit();
            if (btnPressed(Btn::A)) {
                on_ = ! on_;
                icon_.setVisible(! icon_.visible());
                setBrightness();
            }
            if (btnPressed(Btn::Left)) {
                if (brightness_.value() > brightness_.min()) {
                    brightness_.setValue(brightness_.value() - 1);
                    setBrightness();
                }
            }
            if (btnPressed(Btn::Right)) {
                if (brightness_.value() < brightness_.max()) {
                    brightness_.setValue(brightness_.value() + 1);
                    setBrightness();
                }
            }
            if (!on_) {
                if (icon_.visible() && ! btnDown(Btn::Start)) {
                    icon_.setVisible(false);
                    setBrightness();
                }
                if (! icon_.visible() && btnDown(Btn::Start)) {
                    icon_.setVisible(true);
                    setBrightness();
                }
            }
        }

    private:

        static constexpr gpio::Pin FLASHLIGHT_PIN = 47;
        static constexpr uint32_t PWM_SLICE = 11;


        void setBrightness() {
#if defined(PLATFORM_RP2350)
            uint8_t level = brightness_.value() << 4 | brightness_.value();
            if (! icon_.visible())
                level = 0;
            if (level == 0) {
                gpio_set_function(FLASHLIGHT_PIN, GPIO_FUNC_SIO);
                gpio::outputLow(FLASHLIGHT_PIN);
            } else if (level == 255) {
                gpio_set_function(FLASHLIGHT_PIN, GPIO_FUNC_SIO);
                gpio::outputHigh(FLASHLIGHT_PIN);
            } else {
                gpio_set_function(FLASHLIGHT_PIN, GPIO_FUNC_PWM);
                pwm_set_wrap(PWM_SLICE, 256);
                pwm_set_clkdiv(PWM_SLICE, 64.0f);
                pwm_set_chan_level(PWM_SLICE, PWM_CHAN_B, level);
                pwm_set_enabled(PWM_SLICE, true);
            }
#endif
        }

        ui::Image icon_{160 - 32, 60, Icon{assets::icons_64::flashlight}};
        ui::ProgressBar brightness_{Rect::XYWH(20, 170, 280, 20), 0, 15, 8};

        bool on_ = true;

    }; // rckid::Flashlight

} // namespace rckid