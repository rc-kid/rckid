#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/ui/animation.h>
#include <rckid/capabilities/pedometer.h>

#include <rckid/apps/dialogs/info_dialog.h>

#include <assets/OpenDyslexic64.h>
#include <assets/icons_64.h>

namespace rckid {

    /** Step counter.
     
        A very simple step counter app that shows the current step count on the screen. 

        TODO: daily stats & history, goals, etc.
     */
    class Steps : public ui::App<void> {
    public:
        Steps():
            pedometer_{Pedometer::instance()}
        {
            using namespace ui;
            // when not available, show error message and exit the app
            if (pedometer_ == nullptr | true) {
                InfoDialog::error("Not available", "Step counter not supported by the device");
                exit();
                return;
            }
            icon_ = addChild(new Image())
                << SetRect(Rect::XYWH(0, 60, 320, 64))
                << SetBitmap(assets::icons_64::footprint);
            steps_ = addChild(new Label())
                << SetRect(Rect::XYWH(0, 130, 320, 64))
                << SetText("Steps Counter")
                << SetFont(assets::OpenDyslexic64)
                << SetHAlign(HAlign::Center);
        }

    protected:

        void onLoopStart() override {
            animate()
                << ui::FlyIn(icon_, 600)
                << ui::FlyIn(steps_, 500);
        }

        void loop() override {
            using namespace ui;
            App<void>::loop();
            with(steps_) << SetText(STR(pedometer_->count()));
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                animate()
                    << ui::FlyOut(icon_, 500)
                    << ui::FlyOut(steps_, 600);
                waitUntilIdle();
                exit();
            }
        }

    private:

        ui::Image * icon_ = nullptr;
        ui::Label * steps_ = nullptr;

        Pedometer * pedometer_;

    }; // rckid::Steps
    

} // namespace rckid