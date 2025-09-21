#pragma once

#include "../../ui/form.h"
#include "../../ui/progressbar.h"
#include "../../utils/interpolation.h"

namespace rckid {
    class Slider : public ui::Form<int32_t> {
    public:
        String name() const override { return "SliderDialog"; }

        using ChangeCallback = std::function<void(int32_t)>;

        Slider(Icon icon, String title, int32_t min, int32_t max, int32_t value, ChangeCallback cb = nullptr, int32_t step = 1):
            ui::Form<int32_t>{Rect::XYWH(0, 144, 320, 96), /* raw */ true},
            cb_{cb},
            step_{step},
            icon_{Rect::WH(64, 64), icon},
            title_{ 96, 0, std::move(title)},
            slider_{Rect::XYWH(96, 58, 200, 20), min, max, value} {
            icon_.setTransparentColor(ColorRGB::Black());
            title_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            g_.addChild(icon_);
            g_.addChild(title_);
            g_.addChild(slider_);
        }

        void setAnimation(Point iconStart, Point textStart, uint32_t durationMs = 500) {
            t_.setDuration(durationMs);
            icon_.setPos(iconStart);
            title_.setPos(textStart);
            aIcon_ = Animation2D{iconStart, Point{16, 16}, interpolation::cosine};
            aText_ = Animation2D{textStart, Point{96, 0}, interpolation::cosine};
            t_.start();
            slider_.setVisible(false);
        }

        void update() override {
            ui::Form<int32_t>::update();
            if (! t_.running()) {
                // when back or down is pressed, return from the player mode
                if (btnPressed(Btn::B) || btnPressed(Btn::A) || btnPressed(Btn::Down)) {
                    if (t_.duration() != 0) {
                        exitAtEnd_ = true;
                        aIcon_.reverse();
                        aText_.reverse();
                        t_.start();
                        slider_.setVisible(false);
                    } else {
                        exit();
                    }
                }
                if (btnPressed(Btn::Left)) {
                    if (slider_.value() == slider_.min())
                        return;
                    slider_.setValue(slider_.value() - step_);
                    if (cb_ != nullptr)
                        cb_(slider_.value());
                }
                if (btnPressed(Btn::Right)) {
                    if (slider_.value() == slider_.max())
                        return;
                    slider_.setValue(slider_.value() + step_);
                    if (cb_ != nullptr)
                        cb_(slider_.value());
                }
            }
        }

        void draw() override {
            if (t_.running()) {
                t_.update();
                icon_.setPos(aIcon_.update(t_));
                title_.setPos(aText_.update(t_));
                if (! t_.running()) {
                    if (exitAtEnd_)
                        exit();
                    else
                        slider_.setVisible(true);
                }
            }
            ui::Form<int32_t>::draw();
        }

    private:

        ChangeCallback cb_;
        int32_t step_ = 1;
        ui::Image icon_;
        ui::Label title_;
        ui::ProgressBar slider_;
        Timer t_{0};
        Animation2D aIcon_;
        Animation2D aText_;
        bool exitAtEnd_ = false;

    }; // rckid::Slider

} // namespace rckid