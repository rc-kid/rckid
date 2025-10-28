#pragma once

#include "../../ui/form.h"
#include "../../ui/style.h"
#include "../../ui/progressbar.h"


namespace rckid {

    /** Very basic RGB color picker. 
     */
    class ColorPicker : public ui::Form<ColorRGB> {
    public:

        String name() const override { return "ColorPicker"; }

        ColorPicker(ColorRGB initialColor = ui::Style::accentFg()):
            ui::Form<ColorRGB>{Rect::XYWH(0, 144, 320, 96), /* raw */ true},
            red_{Rect::XYWH(10, 12, 200, 16), 0, 255, initialColor.r()},
            green_{Rect::XYWH(10, 40, 200, 16), 0, 255, initialColor.g()},
            blue_{Rect::XYWH(10, 68, 200, 16), 0, 255, initialColor.b()},
            preview_{Rect::XYWH(238, 12, 72, 72)} {
            g_.addChild(red_);
            g_.addChild(green_);
            g_.addChild(blue_);
            g_.addChild(preview_);
            red_.setFg(ColorRGB::Red());
            green_.setFg(ColorRGB::Green().withAlpha(128));
            blue_.setFg(ColorRGB::Blue().withAlpha(128));
            updatePreview();
        }

        void update() override {
            ui::Form<ColorRGB>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::A)) {
                exit(ColorRGB{static_cast<uint8_t>(red_.value()),
                              static_cast<uint8_t>(green_.value()),
                              static_cast<uint8_t>(blue_.value())});
            }
            switch (activeColor_) {
                case 0:
                    checkColor(red_);
                    break;
                case 1:
                    checkColor(green_);
                    break;
                case 2:
                    checkColor(blue_);
                    break;
            }
            if (btnPressed(Btn::Up))
                highlight((activeColor_ + 2) % 3);
            if (btnPressed(Btn::Down))
                highlight((activeColor_ + 1) % 3);
        }

        void draw() override {
            ui::Form<ColorRGB>::draw();
            ColorRGB c{static_cast<uint8_t>(red_.value()),
                        static_cast<uint8_t>(green_.value()),
                        static_cast<uint8_t>(blue_.value())};
            preview_.setBg(c);
        }

    private:

        void checkColor(ui::ProgressBar & bar) {
            if (btnPressed(Btn::Left))
                bar.setValue(bar.value() - 8);
            if (btnPressed(Btn::Right))
                bar.setValue(bar.value() + 8);
        }

        void highlight(uint32_t colorIndex) {
            activeColor_ = colorIndex;
            red_.setFg(ColorRGB::Red().withAlpha(activeColor_ == 0 ? 255 : 128));
            green_.setFg(ColorRGB::Green().withAlpha(activeColor_ == 1 ? 255 : 128));
            blue_.setFg(ColorRGB::Blue().withAlpha(activeColor_ == 2 ? 255 : 128));
        }


        void updatePreview() {
            ColorRGB c{static_cast<uint8_t>(red_.value()),
                        static_cast<uint8_t>(green_.value()),
                        static_cast<uint8_t>(blue_.value())};
            preview_.setBg(c);
        }

        ui::ProgressBar red_;
        ui::ProgressBar green_;
        ui::ProgressBar blue_;
        ui::Panel preview_;
        uint32_t activeColor_ = 0;
    }; // rckid::ColorPicker


} // namespace rckid
