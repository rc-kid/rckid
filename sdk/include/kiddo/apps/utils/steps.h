#pragma once

#include <kiddo/ui/app.h>
#include <kiddo/ui/label.h>
#include <kiddo/ui/image.h>
#include <kiddo/ui/animation.h>
#include <kiddo/capabilities/pedometer.h>

#include <kiddo/apps/dialogs/info_dialog.h>

#include <assets/OpenDyslexic64.h>
#include <assets/icons_64.h>

namespace kiddo {

    /** Step counter.
     
        A very simple step counter app that shows the current step count on the screen. 

        TODO: daily stats & history, goals, etc.
     */
    class Steps : public ui::App<void> {
    public:

        String name() const override { return "Steps"; }

        Steps():
            pedometer_{Pedometer::instance()}
        {
            using namespace ui;
            // when not available, show error message and exit the app
            if (pedometer_ == nullptr) {
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

        void loop() override {
            using namespace ui;
            App<void>::loop();
            with(steps_) << SetText(STR(pedometer_->count()));
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                return exit();
        }

    private:

        ui::Image * icon_ = nullptr;
        ui::Label * steps_ = nullptr;

        Pedometer * pedometer_;

    }; // kiddo::Steps
    

} // namespace kiddo