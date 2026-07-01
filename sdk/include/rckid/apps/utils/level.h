#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/panel.h>

namespace rckid {

    /** Simple bubble level using the accelerometer.
     */
    class Level : public ui::App<void> {
    public:

        String name() const override { return "Level"; }

        Level():
            ui::App<void>{}
        {
            using namespace ui;
            x_ = addChild(new ui::Panel{})
                << SetRect(Rect::XYWH(160 - 5, 120 - 5, 10, 10))
                << SetBg(ui::Style::defaultStyle().accentFg());
        }

    protected:

        void loop() override {
            using namespace ui;
            App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();

            Point3D acc = accel();
            with(x_)
                << SetPosition(160 - 5 + acc.x / 164, 120 - 5 + acc.y / 164); 
        }

    private:
        ui::Panel * x_;

    }; // Level
} // namespace rckid