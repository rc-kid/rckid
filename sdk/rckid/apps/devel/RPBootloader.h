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

#if (defined PLATFORM_RP2350)
#include <pico/bootrom.h>
#endif

namespace rckid {

    /** RP Bootloader.
     */
    class RPBootloader : public ui::Form<void> {
    public:

        String name() const override { return "RPBootloader"; }

        RPBootloader():
            ui::Form<void>{}
        {
            icon_ = g_.addChild(new ui::Image{Icon{assets::icons_64::microchip}});
            status_ = g_.addChild(new ui::Label{Rect::XYWH(0, 130, 320, 64), "USB bootloader"});
            status_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            icon_->setTransparent(true);
            icon_->setPos(160 - icon_->width() / 2, 60);
            status_->setHAlign(HAlign::Center);
        }

    protected:

        void update() override {
            if (enter_) {
#if (defined PLATFORM_RP2350)
                displayWaitUpdateDone();
                rom_reset_usb_boot(0, 0);
#else
                ui::Form<void>::update();
#endif
            }
            // TODO tell AVR to show the uploader lights (and tell it we are in bootloader mode so no pings etc.)
            enter_ = true;
        }

    private:
        ui::Image * icon_;
        ui::Label * status_;
        bool enter_ = false;

    }; // RPBootloader
} // namespace rckid