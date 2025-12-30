#pragma once

#include "../../app.h"
#include "../../filesystem.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/icons_64.h"

namespace rckid {

    /** Very simple about dialog.
     */
    class About : public ui::Form<void> {
    public:

        String name() const override { return "About"; }

        About():
            ui::Form<void>{}
        {
            icon_ = g_.addChild(new ui::Image{Icon{assets::icons_64::info}});
            icon_->setTransparent(true);
            icon_->setPos(160 - icon_->width() / 2, 60);
            status_ = g_.addChild(new ui::Label{Rect::XYWH(0, 130, 320, 20), "RCKid mkIII, v. 0.9.0"});
            TinyTime t{budget()};
            budget_ = g_.addChild(new ui::Label{Rect::XYWH(0, 160, 320, 20), STR("Remaining budget: " << fillLeft(t.hour(), 2, '0') << ":" << fillLeft(t.minute(), 2, '0') << ":" << fillLeft(t.second(), 2, '0'))});
            if (fs::isMounted(fs::Drive::SD))
                sd_ = g_.addChild(new ui::Label{Rect::XYWH(0, 190, 320, 20), STR("SD Card: " << fs::getLabel())});
            else
                sd_ = g_.addChild(new ui::Label{Rect::XYWH(0, 190, 320, 20), "SD Card: Not mounted"});
        }
    protected:

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down) || btnPressed(Btn::A))
                exit();
        }

    private:
        ui::Image * icon_;
        ui::Label * status_;
        ui::Label * budget_;
        ui::Label * sd_;
    }; // About
} // namespace rckid