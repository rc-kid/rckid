#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/ui/progress_bar.h>

#include <assets/icons_64.h>



namespace rckid {

    class Flashlight : public ui::App<void> {
    public:
        String name() const override { return "Flashlight"; }

        Flashlight() {
            using namespace ui;
            icon_ = addChild(new ui::Image{})
                << SetRect(Rect::XYWH(160-32, 60, 64, 64))
                << SetBitmap(assets::icons_64::poo);
            brightness_ = addChild(new ui::ProgressBar{})
                << SetRect(Rect::XYWH(20, 170, 280, 30))
                << SetRange(0, 15)
                << SetValue(8);
        }

        ~Flashlight() override {
            //brightness_->setValue(0);
            //setBrightness();
        }

    protected:

        void onLoopStart() override {
            ui::App<void>::onLoopStart();
            root_.flyIn();
        }

        void loop() {
            ui::App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit();
                // wait for idle to make sure we are exiting from known state
                waitUntilIdle();
                root_.flyOut();
                waitUntilIdle();
            }
            if (btnPressed(Btn::Left)) {
                brightness_->dec();
            }
            if (btnPressed(Btn::Right)) {
                brightness_->inc();
            }
            if (btnPressed(Btn::A)) {
                // TODO turn flashlight on/off
            }

        }

    private:

        ui::Image * icon_;
        ui::ProgressBar * brightness_;

    }; // rckid::Flashlight

} // namespace rckid