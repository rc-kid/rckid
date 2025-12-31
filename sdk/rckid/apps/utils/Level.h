#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/style.h"
#include "../../ui/geometry.h"

namespace rckid {

    /** Simple bubble level using the accelerometer.
     */
    class Level : public ui::Form<void> {
    public:

        String name() const override { return "Level"; }

        Level():
            ui::Form<void>{}
        {
            x_ = g_.addChild(new ui::Rectangle{Rect::XYWH(160 - 5, 120 - 5, 10, 10)});
            x_->setColor(ui::Style::accentFg());
            x_->setFill(true);
        }

    protected:

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
        }

        void draw() override {
            x_->setPos(
                160 - 5 + accelX() / 164,
                120 - 5 + accelY() / 164
            ); 
            ui::Form<void>::draw();
        }

    private:
        ui::Rectangle * x_;

    }; // Level
} // namespace rckid