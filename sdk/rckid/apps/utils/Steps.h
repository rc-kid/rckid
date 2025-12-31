#pragma once

#include "../../app.h"
#include "../../filesystem.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/icons_64.h"
#include "../../assets/fonts/OpenDyslexic64.h"
#include "../../audio/audio.h"
#include "../../utils/ini.h"
#include "../../pim.h"

namespace rckid {

    /** Simple Steps counter.
     */
    class Steps : public ui::Form<void> {
    public:

        String name() const override { return "Steps"; }

        Steps():
            ui::Form<void>{}
        {
            icon_ = g_.addChild(new ui::Image{Icon{assets::icons_64::footprint}});
            status_ = g_.addChild(new ui::Label{Rect::XYWH(0, 130, 320, 64), STR(pedometerCount())});
            status_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            icon_->setTransparent(true);
            icon_->setPos(160 - icon_->width() / 2, 60);
            status_->setHAlign(HAlign::Center);
        }

    protected:

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
        }

        void onSecondTick() override {
            status_->setText(STR(pedometerCount()));
        }

    private:
        ui::Image * icon_;
        ui::Label * status_;

    }; // Alarm
} // namespace rckid