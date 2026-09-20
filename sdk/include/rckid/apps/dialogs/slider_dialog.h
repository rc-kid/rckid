#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/image.h>
#include <rckid/ui/label.h>
#include <rckid/ui/progress_bar.h>
#include <assets/OpenDyslexic64.h>

#include <assets/icons_64.h>


namespace rckid {

    class SliderDialog : public ui::App<int32_t> {
    public:
    
        String name() const override { return "Slider Dialog"; }

        SliderDialog(String title, ImageSource icon, int32_t min, int32_t max, int32_t initial, std::function<void(int32_t)> onChange = nullptr):
            ui::App<int32_t>{Rect::XYWH(0, 140, 320, 100)},
            initial_{initial},
            onChange_{std::move(onChange)}
        {
            using namespace ui;
            icon_ = addChild(new Image())
                << SetRect(Rect::XYWH(0, 0, 100, 100))
                << SetBitmap(std::move(icon))
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Center);
            title_ = addChild(new Label())
                << SetRect(Rect::XYWH(100, 0, 220, 64))
                << SetText(title)
                << SetFont(assets::OpenDyslexic64)
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Left);

            slider_ = addChild(new ProgressBar())
                << SetRect(Rect::XYWH(100, 64, 210, 20))
                << SetRange(min, max)
                << SetValue(initial);

            root_.useBackgroundImage(false);
            root_.setBg(ui::Style::defaultBg());

        }

    private:

        void loop() override {
            ui::App<int32_t>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                if (onChange_)
                    onChange_(initial_);
                exit(static_cast<int32_t>(initial_));
            } else if (btnPressed(Btn::Left)) {
                if (slider_->dec() && onChange_)
                    onChange_(slider_->value());
            } else if (btnPressed(Btn::Right)) {
                if (slider_->inc() && onChange_)
                    onChange_(slider_->value());
            } else if (btnPressed(Btn::A)) {
                exit(slider_->value());
            }
        }

        int32_t initial_;
        std::function<void(int32_t)> onChange_;

        ui::Image * icon_;
        ui::Label * title_;
        ui::ProgressBar * slider_;


    }; // rckid::SliderDialog

} // namespace rckid