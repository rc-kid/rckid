#pragma once

#include "../../app.h"
#include "../../filesystem.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/icons_64.h"
#include "../../assets/fonts/OpenDyslexic64.h"

namespace rckid {

    /** Alarm 
     */
    class Alarm : public ui::Form<void> {
    public:

        String name() const override { return "Alarm"; }

        Alarm():
            ui::Form<void>{}
        {
            icon_ = g_.addChild(new ui::Image{Icon{assets::icons_64::alarm_clock}});
            status_ = g_.addChild(new ui::Label{Rect::XYWH(0, 130, 320, 64), "Wake Up!"});
            status_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            icon_->setTransparent(true);
            icon_->setPos(160 - icon_->width() / 2, 60);
            status_->setHAlign(HAlign::Center);
        }

        ~Alarm() {
        }

        static void check() {
            if (alarm())
                App::run<Alarm>();
        }

    protected:

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit();
            }
        }

    private:
        ui::Image * icon_;
        ui::Label * status_;

    }; // Alarm
} // namespace rckid