#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/ui/progress_bar.h>
#include <rckid/capabilities/led.h>

#include <assets/icons_64.h>



namespace rckid {

    class Flashlight : public ui::App<void> {
    public:
        String name() const override { return "Flashlight"; }

        Flashlight():
            led_{LED::instance()}
        {
            using namespace ui;
            if (led_ == nullptr) {
                InfoDialog::error("Not available", "Step counter not supported by the device");
                exit();
                return;
            }
            icon_ = addChild(new ui::Image{})
                << SetRect(Rect::XYWH(160-32, 60, 64, 64))
                << SetBitmap(assets::icons_64::flashlight);
            brightness_ = addChild(new ui::ProgressBar{})
                << SetRect(Rect::XYWH(20, 170, 280, 30))
                << SetRange(0, 15)
                << SetValue(8);
            led_->setBrightness(128);
            led_->setActive(true);            
        }

        ~Flashlight() override {
            if (led_)
                led_->setActive(false);
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
                updateBrightness();
            }
            if (btnPressed(Btn::Right)) {
                brightness_->inc();
                updateBrightness();
            }
            if (btnPressed(Btn::A)) {
                on_ = ! on_;
                icon_->setVisibility(on_);
                updateBrightness();
            }
            if (on_) {
                if (icon_->visible() && btnDown(Btn::Start)) {
                    icon_->setVisibility(false);
                    updateBrightness();
                }
                if (! icon_->visible() && ! btnDown(Btn::Start)) {
                    icon_->setVisibility(true);
                    updateBrightness();
                }
            } else {
                if (icon_->visible() && ! btnDown(Btn::Start)) {
                    icon_->setVisibility(false);
                    updateBrightness();
                }
                if (! icon_->visible() && btnDown(Btn::Start)) {
                    icon_->setVisibility(true);
                    updateBrightness();
                }
            }
        }

    private:

        void updateBrightness() {
            uint8_t level = brightness_->value() << 4 | brightness_->value();
            if (! icon_->visible())
                level = 0;
            led_->setBrightness(level);
        }

        LED * led_; 

        ui::Image * icon_;
        ui::ProgressBar * brightness_;
        bool on_ = true;


    }; // rckid::Flashlight

} // namespace rckid