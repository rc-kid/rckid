#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../assets/fonts/OpenDyslexic128.h"


namespace rckid {

    class Flashlight : public ui::Form<void> {
    public:

        String name() const override { return "Flashlight"; }

        Flashlight() : ui::Form<void>{} {
            gpio::outputHigh(47);
            gpio::outputHigh(46);
        }

        ~Flashlight() override {
            gpio::setAsInput(47);
            gpio::setAsInput(46);
        }

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::Down) || btnPressed(Btn::B))
                exit();
        }

    }; // rckid::Flashlight

} // namespace rckid